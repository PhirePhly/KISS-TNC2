#include "tnc2.h"

#ifndef HOST_TEST
__sfr __at (A_DATA_PORT) sio_a_data;
__sfr __at (A_CTRL_PORT) sio_a_ctrl;
__sfr __at (B_DATA_PORT) sio_b_data;
__sfr __at (B_CTRL_PORT) sio_b_ctrl;

#define DECLARE_EXT(hex) __sfr __at (0x##hex) ext_##hex
DECLARE_EXT(A0); DECLARE_EXT(A1); DECLARE_EXT(A2); DECLARE_EXT(A3);
DECLARE_EXT(A4); DECLARE_EXT(A5); DECLARE_EXT(A6); DECLARE_EXT(A7);
DECLARE_EXT(A8); DECLARE_EXT(A9); DECLARE_EXT(AA); DECLARE_EXT(AB);
DECLARE_EXT(AC); DECLARE_EXT(AD); DECLARE_EXT(AE); DECLARE_EXT(AF);
DECLARE_EXT(B0); DECLARE_EXT(B1); DECLARE_EXT(B2); DECLARE_EXT(B3);
DECLARE_EXT(B4); DECLARE_EXT(B5); DECLARE_EXT(B6); DECLARE_EXT(B7);
DECLARE_EXT(B8); DECLARE_EXT(B9); DECLARE_EXT(BA); DECLARE_EXT(BB);
DECLARE_EXT(BC); DECLARE_EXT(BD); DECLARE_EXT(BE); DECLARE_EXT(BF);

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

void hardware_write_extension(uint8_t port, uint8_t value)
{
#define EXT_CASE(hex) case 0x##hex: ext_##hex = value; break
    switch (port) {
    EXT_CASE(A0); EXT_CASE(A1); EXT_CASE(A2); EXT_CASE(A3);
    EXT_CASE(A4); EXT_CASE(A5); EXT_CASE(A6); EXT_CASE(A7);
    EXT_CASE(A8); EXT_CASE(A9); EXT_CASE(AA); EXT_CASE(AB);
    EXT_CASE(AC); EXT_CASE(AD); EXT_CASE(AE); EXT_CASE(AF);
    EXT_CASE(B0); EXT_CASE(B1); EXT_CASE(B2); EXT_CASE(B3);
    EXT_CASE(B4); EXT_CASE(B5); EXT_CASE(B6); EXT_CASE(B7);
    EXT_CASE(B8); EXT_CASE(B9); EXT_CASE(BA); EXT_CASE(BB);
    EXT_CASE(BC); EXT_CASE(BD); EXT_CASE(BE); EXT_CASE(BF);
    default: break;
    }
#undef EXT_CASE
}

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
