#include "tnc2.h"

static void discard_rx(void)
{
    if (g_state.rx_allocated) {
        buffer_free_chain(g_state.rx_head);
    }
    g_state.rx_allocated = 0;
    g_state.rx_head = INVALID_BUFFER;
    g_state.rx_current = INVALID_BUFFER;
}

void modem_receive_byte(uint8_t byte)
{
    BufferRef current;

    if (g_state.rx_flushing) {
        return;
    }

    if (!g_state.rx_allocated) {
        BufferRef ref = buffer_alloc();
        if (ref == INVALID_BUFFER) {
            g_state.rx_flushing = 1;
            hardware_sta(true);
            return;
        }
        g_state.rx_head = ref;
        g_state.rx_current = ref;
        g_state.rx_allocated = 1;
        current = g_state.rx_current;
        if (!buffer_put(&current, 0)) {
            discard_rx();
            g_state.rx_flushing = 1;
            hardware_sta(true);
            return;
        }
        g_state.rx_current = current;
    }

    current = g_state.rx_current;
    if (!buffer_put(&current, byte)) {
        discard_rx();
        g_state.rx_flushing = 1;
        hardware_sta(true);
    } else {
        g_state.rx_current = current;
    }
}

void modem_end_frame(uint8_t rr1)
{
    bool good = (rr1 & RR1_END_OF_FRAME) != 0 &&
                (rr1 & (RR1_RX_OVERRUN | RR1_CRC_ERROR)) == 0;

    if (good && g_state.rx_allocated) {
        BufferRef frame = g_state.rx_head;
        if (!out_queue_push(frame)) {
            buffer_free_chain(frame);
        }
        g_state.rx_allocated = 0;
        g_state.rx_head = INVALID_BUFFER;
        g_state.rx_current = INVALID_BUFFER;
    } else {
        discard_rx();
    }
    g_state.rx_flushing = 0;
    g_state.rx_state = 0;
}

static bool channel_busy(void)
{
    if ((g_state.soft_dcd & DCD_SOFTWARE) && g_state.rx_state) {
        return true;
    }
    if ((g_state.soft_dcd & DCD_HARDWARE) && (g_state.a_rr0 & RR0_DCD)) {
        return true;
    }
    return false;
}

static void start_modem_frame(void)
{
    BufferRef frame;
    BufferRef chain;
    uint8_t byte;

    hardware_irq_disable();
    if (!tx_queue_pop(&frame)) {
        g_state.tx_state = TX_IDLE;
        hardware_irq_enable();
        return;
    }
    g_state.tx_chain = frame;
    g_state.tx_state = TX_SENDING;
    hardware_a_ctrl_write(0x80);
    chain = g_state.tx_chain;
    if (!buffer_get(&chain, &byte)) {
        if (g_state.tx_outstanding) {
            --g_state.tx_outstanding;
        }
        g_state.tx_started = 0;
        g_state.tx_chain = INVALID_BUFFER;
        hardware_a_ctrl_write(0x28);
        g_state.tx_timer = g_state.txtail;
        g_state.tx_state = TX_TAIL;
        hardware_irq_enable();
        return;
    }
    g_state.tx_chain = chain;
    hardware_a_data_write(byte);
    g_state.tx_started = 1;
    hardware_a_ctrl_write(0xC0);
    hardware_irq_enable();
}

void modem_service(void)
{
    uint8_t random;

    switch ((TxState)g_state.tx_state) {
    case TX_IDLE:
        if (!g_state.tx_outstanding) {
            return;
        }
        random = (uint8_t)(hardware_random() << 1);
        if (g_state.persistence < random) {
            g_state.tx_timer = g_state.slottime;
            g_state.tx_state = TX_SLOT_WAIT;
            return;
        }
        if (!g_state.full_duplex && channel_busy()) {
            g_state.tx_timer = g_state.slottime;
            g_state.tx_state = TX_SLOT_WAIT;
            return;
        }
        g_state.tx_timer = g_state.txdelay;
        hardware_ptt(true);
        g_state.tx_state = TX_DELAY;
        return;

    case TX_SLOT_WAIT:
        if (!g_state.tx_timer) {
            g_state.tx_state = TX_IDLE;
        }
        return;

    case TX_DELAY:
        if (!g_state.tx_timer) {
            g_state.tx_state = TX_CTS_WAIT;
        }
        return;

    case TX_CTS_WAIT:
        if (g_state.cts_control && !(g_state.a_rr0 & RR0_CTS)) {
            return;
        }
        start_modem_frame();
        return;

    case TX_SENDING:
        return;

    case TX_TAIL:
        if (!g_state.tx_timer) {
            hardware_ptt(false);
            g_state.tx_state = TX_IDLE;
        }
        return;
    }
}

void host_service(void)
{
    BufferRef frame;

    if (g_state.host_out_started ||
        !(hardware_b_ctrl_read() & RR0_TX_EMPTY)) {
        return;
    }

    hardware_irq_disable();
    hardware_con(false);
    if (!out_queue_pop(&frame)) {
        hardware_irq_enable();
        return;
    }
    g_state.host_chain = frame;
    g_state.host_out_started = 1;
    g_state.host_escape = 0;
    hardware_con(true);
    hardware_b_data_write(FEND);
    hardware_irq_enable();
}
