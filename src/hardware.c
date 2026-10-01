#include "tnc2.h"

#ifndef HOST_TEST
__sfr __at (A_DATA_PORT) sio_a_data;
__sfr __at (A_CTRL_PORT) sio_a_ctrl;
__sfr __at (B_DATA_PORT) sio_b_data;
__sfr __at (B_CTRL_PORT) sio_b_ctrl;

static const uint8_t a_init[] = {
    0x18, 4, 0x20, 1, 0x1B, 7, 0x7E, 5, A_WR5_DEFAULT, 3, 0xC9
};
static const uint8_t b_init[] = {
    0x18, 4, 0x44, 2, 0x00, 3, 0xE1, 5, B_WR5_DEFAULT, 1, 0x1F
};

void hardware_init(void)
{
    uint8_t i;

    (void)sio_a_ctrl;
    for (i = 0; i < (uint8_t)sizeof(a_init); ++i) {
        sio_a_ctrl = a_init[i];
    }
    (void)sio_b_ctrl;
    for (i = 0; i < (uint8_t)sizeof(b_init); ++i) {
        sio_b_ctrl = b_init[i];
    }
}

uint8_t hardware_a_ctrl_read(void) { return sio_a_ctrl; }
uint8_t hardware_b_ctrl_read(void) { return sio_b_ctrl; }
uint8_t hardware_a_data_read(void) { return sio_a_data; }
uint8_t hardware_b_data_read(void) { return sio_b_data; }
void hardware_a_ctrl_write(uint8_t value) { sio_a_ctrl = value; }
void hardware_b_ctrl_write(uint8_t value) { sio_b_ctrl = value; }
void hardware_a_data_write(uint8_t value) { sio_a_data = value; }
void hardware_b_data_write(uint8_t value) { sio_b_data = value; }

void hardware_sta(bool on)
{
    sio_a_ctrl = 5;
    if (on) {
        g_state.a_wr5 &= (uint8_t)~WR5_LED;
    } else {
        g_state.a_wr5 |= WR5_LED;
    }
    sio_a_ctrl = g_state.a_wr5;
}

void hardware_con(bool on)
{
    sio_b_ctrl = 5;
    g_state.b_wr5 = on ? 0x6Au : B_WR5_DEFAULT;
    sio_b_ctrl = g_state.b_wr5;
}

void hardware_ptt(bool on)
{
    hardware_irq_disable();
    sio_a_ctrl = 5;
    if (on) {
        g_state.a_wr5 |= WR5_RTS;
    } else {
        g_state.a_wr5 &= (uint8_t)~WR5_RTS;
    }
    sio_a_ctrl = g_state.a_wr5;
    hardware_irq_enable();
}
#endif
