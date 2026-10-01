#include "tnc2.h"

static void set_sta(bool on)
{
#ifndef HOST_TEST
    hardware_sta(on);
#else
    (void)on;
#endif
}

static bool input_put(uint8_t byte)
{
    BufferRef current;

    if (!g_state.in_allocated) {
        BufferRef ref = buffer_alloc();
        if (ref == INVALID_BUFFER) {
            return false;
        }
        g_state.in_head = ref;
        g_state.in_current = ref;
        g_state.in_allocated = 1;
    }
    current = g_state.in_current;
    if (!buffer_put(&current, byte)) {
        return false;
    }
    g_state.in_current = current;
    return true;
}

static void finish_data_frame(void)
{
    BufferRef frame;

    if (!g_state.in_allocated) {
        g_state.kiss_state = KISS_COMMAND;
        set_sta(false);
        return;
    }

    frame = g_state.in_head;
    if (!input_put(0) || !tx_queue_push(frame)) {
        buffer_free_chain(frame);
    }
    g_state.in_allocated = 0;
    g_state.in_head = INVALID_BUFFER;
    g_state.in_current = INVALID_BUFFER;
    g_state.kiss_state = KISS_COMMAND;
    set_sta(false);
}

void kiss_reset(void)
{
    if (g_state.in_allocated) {
        buffer_free_chain(g_state.in_head);
    }
    g_state.in_allocated = 0;
    g_state.in_head = INVALID_BUFFER;
    g_state.in_current = INVALID_BUFFER;
    g_state.kiss_state = KISS_HUNT;
    set_sta(false);
}

void kiss_abort_frame(void)
{
    kiss_reset();
}

static void receive_command(uint8_t byte)
{
    uint8_t command;

    if (byte == FEND) {
        return;
    }
    set_sta(true);
    command = byte & 0x0Fu;
    switch (command) {
    case 0:
        g_state.kiss_state = KISS_DATA;
        break;
    case 1:
        g_state.kiss_state = KISS_TXDELAY;
        break;
    case 2:
        g_state.kiss_state = KISS_PERSISTENCE;
        break;
    case 3:
        g_state.kiss_state = KISS_SLOTTIME;
        break;
    case 4:
        g_state.kiss_state = KISS_TXTAIL;
        break;
    case 5:
        g_state.kiss_state = KISS_FULLDUPLEX;
        break;
    case 6:
        g_state.kiss_state = KISS_HARDWARE;
        break;
    default:
        kiss_reset();
        break;
    }
}

static void receive_hardware(uint8_t byte)
{
    if (byte >= 0xFEu) {
        g_state.cts_control = byte - 0xFEu;
    } else if (byte >= 0xFCu) {
        /* Reserved by the original firmware. */
    } else if (byte >= 0xF8u) {
        g_state.soft_dcd = byte - 0xF8u;
    } else if (byte < 0x20u) {
        g_state.hardware_port = byte + 0xA0u;
        g_state.kiss_state = KISS_HARDWARE_DATA;
        return;
    }
    kiss_reset();
}

void kiss_receive_byte(uint8_t byte)
{
    switch ((KissState)g_state.kiss_state) {
    case KISS_HUNT:
        set_sta(false);
        if (byte == FEND) {
            g_state.kiss_state = KISS_COMMAND;
        }
        break;

    case KISS_COMMAND:
        receive_command(byte);
        break;

    case KISS_DATA:
        if (byte == FEND) {
            finish_data_frame();
        } else if (byte == FESC) {
            g_state.kiss_state = KISS_ESCAPE;
        } else if (!input_put(byte)) {
            kiss_reset();
        }
        break;

    case KISS_ESCAPE:
        if (byte == TFESC) {
            if (!input_put(FESC)) {
                kiss_reset();
                break;
            }
        } else if (byte == TFEND) {
            if (!input_put(FEND)) {
                kiss_reset();
                break;
            }
        }
        g_state.kiss_state = KISS_DATA;
        break;

    case KISS_TXDELAY:
        g_state.txdelay = byte;
        kiss_reset();
        break;
    case KISS_PERSISTENCE:
        g_state.persistence = byte;
        kiss_reset();
        break;
    case KISS_SLOTTIME:
        g_state.slottime = byte;
        kiss_reset();
        break;
    case KISS_TXTAIL:
        g_state.txtail = byte;
        kiss_reset();
        break;
    case KISS_FULLDUPLEX:
        g_state.full_duplex = byte;
        kiss_reset();
        break;
    case KISS_HARDWARE:
        receive_hardware(byte);
        break;
    case KISS_HARDWARE_DATA:
#ifndef HOST_TEST
        hardware_write_extension(g_state.hardware_port, byte);
#endif
        kiss_reset();
        break;
    default:
        kiss_reset();
        break;
    }
}
