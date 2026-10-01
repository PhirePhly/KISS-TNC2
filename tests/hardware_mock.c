#include "hardware_mock.h"

#include <assert.h>

uint8_t mock_b_output[512];
uint16_t mock_b_output_length;

static uint8_t a_ctrl;
static uint8_t b_ctrl = RR0_TX_EMPTY;
static uint8_t a_data;
static uint8_t b_data;

void mock_hardware_reset(void)
{
    mock_b_output_length = 0;
    a_ctrl = 0;
    b_ctrl = RR0_TX_EMPTY;
    a_data = 0;
    b_data = 0;
}

void hardware_init(void) {}
void hardware_set_im2(void) {}
void hardware_irq_disable(void) {}
void hardware_irq_enable(void) {}
uint8_t hardware_random(void) { return 0; }
uint8_t hardware_a_ctrl_read(void) { return a_ctrl; }
uint8_t hardware_b_ctrl_read(void) { return b_ctrl; }
uint8_t hardware_a_data_read(void) { return a_data; }
uint8_t hardware_b_data_read(void) { return b_data; }
void hardware_a_ctrl_write(uint8_t value) { a_ctrl = value; }
void hardware_b_ctrl_write(uint8_t value) { b_ctrl = value; }
void hardware_a_data_write(uint8_t value) { a_data = value; }

void hardware_b_data_write(uint8_t value)
{
    assert(mock_b_output_length < sizeof(mock_b_output));
    mock_b_output[mock_b_output_length++] = value;
    b_data = value;
}

void hardware_sta(bool on) { (void)on; }
void hardware_con(bool on) { (void)on; }
void hardware_ptt(bool on) { (void)on; }
