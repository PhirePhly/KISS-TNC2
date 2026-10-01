#include "tnc2.h"

#ifdef HOST_TEST
volatile FirmwareState g_state;
volatile BufferRef g_tx_queue[QUEUE_SIZE];
volatile BufferRef g_out_queue[QUEUE_SIZE];
Buffer g_buffers[BUFFER_COUNT];
#else
volatile FirmwareState __at (0x8000) g_state;
volatile BufferRef __at (0x8100) g_tx_queue[QUEUE_SIZE];
volatile BufferRef __at (0x8300) g_out_queue[QUEUE_SIZE];
Buffer __at (0x8500) g_buffers[BUFFER_COUNT];
#endif

void state_reset(void)
{
    uint8_t *p = (uint8_t *)&g_state;
    uint16_t i;

    for (i = 0; i < (uint16_t)sizeof(g_state); ++i) {
        p[i] = 0;
    }

    g_state.txdelay = 30;
    g_state.persistence = 63;
    g_state.slottime = 10;
    g_state.txtail = 2;
    g_state.software_dcd = 1;
    g_state.a_rr0 = RR0_CTS;
    g_state.a_wr5 = A_WR5_DEFAULT;
    g_state.b_wr5 = B_WR5_DEFAULT;
    /* Match v.7: a host may begin with a command or one or more FENDs. */
    g_state.kiss_state = KISS_COMMAND;
    g_state.rx_head = INVALID_BUFFER;
    g_state.rx_current = INVALID_BUFFER;
    g_state.in_head = INVALID_BUFFER;
    g_state.in_current = INVALID_BUFFER;
    g_state.host_chain = INVALID_BUFFER;
    g_state.tx_chain = INVALID_BUFFER;
}
