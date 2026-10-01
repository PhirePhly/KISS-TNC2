#include "tnc2.h"

void isr_b_tx(void)
{
    uint8_t byte;
    BufferRef chain;

    if (g_state.host_escape) {
        if (g_state.host_escape_byte) {
            hardware_b_data_write(g_state.host_escape_byte);
            g_state.host_escape = 0;
            return;
        }
        g_state.host_out_started = 0;
        g_state.host_escape = 0;
        hardware_b_ctrl_write(0x28);
        return;
    }

    chain = g_state.host_chain;
    if (!buffer_get(&chain, &byte)) {
        g_state.host_chain = INVALID_BUFFER;
        hardware_b_data_write(FEND);
        g_state.host_escape = 1;
        g_state.host_escape_byte = 0;
        return;
    }
    g_state.host_chain = chain;

    if (byte == FESC) {
        hardware_b_data_write(FESC);
        g_state.host_escape = 1;
        g_state.host_escape_byte = TFESC;
    } else if (byte == FEND) {
        hardware_b_data_write(FESC);
        g_state.host_escape = 1;
        g_state.host_escape_byte = TFEND;
    } else {
        hardware_b_data_write(byte);
    }
}

void isr_b_ext(void)
{
    uint8_t rr0;
    uint8_t level;

    hardware_b_ctrl_write(0x10);
    rr0 = hardware_b_ctrl_read();
    level = (rr0 & RR0_SYNC_HUNT) ? 1u : 0u;

    if (rr0 & RR0_BREAK_ABORT) {
        g_state.host_break = 1;
        (void)hardware_b_data_read();
    } else if (g_state.host_break) {
        g_state.host_break = 0;
        (void)hardware_b_data_read();
    }

    if (level == g_state.tick_level) {
        return;
    }
    g_state.tick_level = level;
    ++g_state.tick_divider;
    if (g_state.tick_divider == 12u) {
        g_state.tick_divider = 0;
        if (g_state.tx_timer) {
            --g_state.tx_timer;
        }
    }
}

void isr_b_rx(void)
{
    uint8_t rr1;
    uint8_t byte;

    (void)hardware_b_ctrl_read();
    hardware_b_ctrl_write(1);
    rr1 = hardware_b_ctrl_read();
    byte = hardware_b_data_read();
    if (rr1 & RR1_FRAMING_ERROR) {
        kiss_abort_frame();
    } else {
        kiss_receive_byte(byte);
    }
}

void isr_b_special(void)
{
    hardware_b_ctrl_write(0x30);
}

void isr_a_tx(void)
{
    uint8_t byte;
    BufferRef chain;

    if (g_state.tx_started) {
        chain = g_state.tx_chain;
        if (buffer_get(&chain, &byte)) {
            g_state.tx_chain = chain;
            hardware_a_data_write(byte);
            return;
        }
        g_state.tx_chain = INVALID_BUFFER;
        g_state.tx_started = 0;
        if (g_state.tx_outstanding) {
            --g_state.tx_outstanding;
        }
        hardware_a_ctrl_write(0x28);
        return;
    }

    if (g_state.tx_outstanding) {
        if (tx_queue_pop(&chain)) {
            g_state.tx_chain = chain;
            hardware_a_ctrl_write(0x80);
            if (buffer_get(&chain, &byte)) {
                g_state.tx_chain = chain;
                hardware_a_data_write(byte);
                g_state.tx_started = 1;
                hardware_a_ctrl_write(0xC0);
                return;
            }
            g_state.tx_chain = INVALID_BUFFER;
            --g_state.tx_outstanding;
        } else {
            /* Recover from impossible queue/count divergence without
             * leaving PTT keyed forever. */
            g_state.tx_outstanding = 0;
        }
    }

    hardware_a_ctrl_write(0x28);
    g_state.tx_timer = g_state.txtail;
    g_state.tx_state = TX_TAIL;
}

void isr_a_ext(void)
{
    uint8_t rr0;

    hardware_a_ctrl_write(0x10);
    rr0 = hardware_a_ctrl_read();
    g_state.a_rr0 = rr0;

    if (rr0 & RR0_SYNC_HUNT) {
        g_state.rx_state = 0;
        g_state.rx_flushing = 0;
    } else if (!g_state.rx_state) {
        g_state.rx_state = 1;
        if (g_state.rx_allocated) {
            buffer_free_chain(g_state.rx_head);
            g_state.rx_allocated = 0;
            g_state.rx_head = INVALID_BUFFER;
            g_state.rx_current = INVALID_BUFFER;
        }
    }
}

void isr_a_rx(void)
{
    modem_receive_byte(hardware_a_data_read());
}

void isr_a_special(void)
{
    uint8_t rr1;

    hardware_a_ctrl_write(1);
    rr1 = hardware_a_ctrl_read();
    modem_end_frame(rr1);

    hardware_a_ctrl_write(0x30);
    (void)hardware_a_data_read();
    hardware_a_ctrl_write(3);
    hardware_a_ctrl_write(0xD9);

    if (!g_state.full_duplex && g_state.tx_state < TX_DELAY &&
        (g_state.soft_dcd & DCD_SOFTWARE)) {
        g_state.tx_state = TX_SLOT_WAIT;
        g_state.tx_timer = g_state.slottime;
    }
}
