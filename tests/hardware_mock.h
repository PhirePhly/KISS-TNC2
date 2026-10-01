#ifndef HARDWARE_MOCK_H
#define HARDWARE_MOCK_H

#include "tnc2.h"

extern uint8_t mock_b_output[512];
extern uint16_t mock_b_output_length;

void mock_hardware_reset(void);

#endif
