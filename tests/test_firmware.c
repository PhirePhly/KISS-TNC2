#include "tnc2.h"
#include "hardware_mock.h"

#include <assert.h>
#include <stdio.h>

static void reset_fixture(void)
{
    state_reset();
    buffers_init();
    kiss_reset();
    mock_hardware_reset();
}

static void send_parameter(uint8_t command, uint8_t value)
{
    kiss_receive_byte(FEND);
    kiss_receive_byte(command);
    kiss_receive_byte(value);
}

static void test_defaults_and_parameters(void)
{
    reset_fixture();
    assert(g_state.txdelay == 33);
    assert(g_state.persistence == 63);
    assert(g_state.slottime == 5);
    assert(g_state.txtail == 3);
    assert(g_state.full_duplex == 0);
    assert(g_state.software_dcd == 1);
    assert(g_state.hardware_dcd == 0);
    assert(g_state.cts_control == 0);

    send_parameter(1, 42);
    send_parameter(2, 127);
    send_parameter(3, 9);
    send_parameter(4, 11);
    send_parameter(5, 1);
    assert(g_state.txdelay == 42);
    assert(g_state.persistence == 127);
    assert(g_state.slottime == 9);
    assert(g_state.txtail == 11);
    assert(g_state.full_duplex == 1);
}

static void send_hardware(uint8_t feature, uint8_t value)
{
    kiss_receive_byte(FEND);
    kiss_receive_byte(6);
    kiss_receive_byte(feature);
    kiss_receive_byte(value);
    kiss_receive_byte(FEND);
}

static void test_hardware_features(void)
{
    reset_fixture();
    send_hardware(KISS_HW_SOFTWARE_DCD, 0);
    send_hardware(KISS_HW_HARDWARE_DCD, 1);
    send_hardware(KISS_HW_CTS, 1);
    assert(g_state.software_dcd == 0);
    assert(g_state.hardware_dcd == 1);
    assert(g_state.cts_control == 1);

    send_hardware(KISS_HW_HARDWARE_DCD, 0);
    send_hardware(KISS_HW_CTS, 0);
    send_hardware(KISS_HW_SOFTWARE_DCD, 1);
    assert(g_state.software_dcd == 1);
    assert(g_state.hardware_dcd == 0);
    assert(g_state.cts_control == 0);
    assert(g_state.kiss_state == KISS_COMMAND);
}

static void test_kiss_unescaping(void)
{
    BufferRef frame;
    uint8_t byte;

    reset_fixture();
    kiss_receive_byte(FEND);
    kiss_receive_byte(0);
    kiss_receive_byte(0x12);
    kiss_receive_byte(FESC);
    kiss_receive_byte(TFEND);
    kiss_receive_byte(FESC);
    kiss_receive_byte(TFESC);
    kiss_receive_byte(FEND);

    assert(g_state.tx_outstanding == 1);
    assert(tx_queue_pop(&frame));
    assert(buffer_get(&frame, &byte) && byte == 0x12);
    assert(buffer_get(&frame, &byte) && byte == FEND);
    assert(buffer_get(&frame, &byte) && byte == FESC);
    assert(!buffer_get(&frame, &byte));
}

static void test_unknown_escape_is_discarded(void)
{
    BufferRef frame;
    uint8_t byte;

    reset_fixture();
    kiss_receive_byte(FEND);
    kiss_receive_byte(0);
    kiss_receive_byte(1);
    kiss_receive_byte(FESC);
    kiss_receive_byte(0x7F);
    kiss_receive_byte(2);
    kiss_receive_byte(FEND);

    assert(tx_queue_pop(&frame));
    assert(buffer_get(&frame, &byte) && byte == 1);
    assert(buffer_get(&frame, &byte) && byte == 2);
    assert(!buffer_get(&frame, &byte));
}

static void test_multibuffer_frame(void)
{
    BufferRef frame;
    uint8_t byte;
    uint16_t i;

    reset_fixture();
    kiss_receive_byte(FEND);
    kiss_receive_byte(0);
    for (i = 0; i < 200; ++i) {
        kiss_receive_byte((uint8_t)(i % 0xBFu));
    }
    kiss_receive_byte(FEND);

    assert(tx_queue_pop(&frame));
    for (i = 0; i < 200; ++i) {
        assert(buffer_get(&frame, &byte));
        assert(byte == (uint8_t)(i % 0xBFu));
    }
    assert(!buffer_get(&frame, &byte));
}

static void test_buffer_exhaustion_recovers(void)
{
    BufferRef refs[BUFFER_COUNT];
    uint16_t i;

    reset_fixture();
    for (i = 0; i < BUFFER_COUNT; ++i) {
        refs[i] = buffer_alloc();
        assert(refs[i] != INVALID_BUFFER);
    }
    assert(buffer_alloc() == INVALID_BUFFER);

    kiss_receive_byte(FEND);
    kiss_receive_byte(0);
    kiss_receive_byte(1);
    assert(g_state.kiss_state == KISS_HUNT);
    assert(!g_state.in_allocated);

    for (i = 0; i < BUFFER_COUNT; ++i) {
        buffer_free(refs[i]);
    }
    assert(buffer_alloc() != INVALID_BUFFER);
}

static void test_queue_wraparound(void)
{
    BufferRef ref;
    uint16_t i;

    reset_fixture();
    for (i = 0; i < 600; ++i) {
        assert(out_queue_push((BufferRef)(i % BUFFER_COUNT)));
        assert(out_queue_pop(&ref));
        assert(ref == (BufferRef)(i % BUFFER_COUNT));
    }
}

static void test_host_slip_encoding(void)
{
    static const uint8_t expected[] = {
        0, FESC, TFEND, FESC, TFESC, FEND
    };
    BufferRef frame;
    BufferRef current;
    uint16_t i;

    reset_fixture();
    frame = buffer_alloc();
    assert(frame != INVALID_BUFFER);
    current = frame;
    assert(buffer_put(&current, 0));
    assert(buffer_put(&current, FEND));
    assert(buffer_put(&current, FESC));
    assert(buffer_put(&current, 0)); /* Sentinel consumed by buffer_get. */

    g_state.host_chain = frame;
    g_state.host_out_started = 1;
    for (i = 0; i < 7; ++i) {
        isr_b_tx();
    }

    assert(mock_b_output_length == sizeof(expected));
    for (i = 0; i < sizeof(expected); ++i) {
        assert(mock_b_output[i] == expected[i]);
    }
    assert(!g_state.host_out_started);
}

int main(void)
{
    test_defaults_and_parameters();
    test_hardware_features();
    test_kiss_unescaping();
    test_unknown_escape_is_discarded();
    test_multibuffer_frame();
    test_buffer_exhaustion_recovers();
    test_queue_wraparound();
    test_host_slip_encoding();
    puts("firmware host tests: PASS");
    return 0;
}
