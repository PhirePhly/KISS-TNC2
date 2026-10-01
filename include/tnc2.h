#ifndef TNC2_H
#define TNC2_H

#ifdef HOST_TEST
#include <stdbool.h>
#include <stdint.h>
#else
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef signed char int8_t;
typedef uint8_t bool;
#define true 1
#define false 0
#endif

#define FEND  0xC0u
#define FESC  0xDBu
#define TFEND 0xDCu
#define TFESC 0xDDu

#define A_DATA_PORT 0xDCu
#define A_CTRL_PORT 0xDDu
#define B_DATA_PORT 0xDEu
#define B_CTRL_PORT 0xDFu

#define RR0_DCD 0x08u
#define RR0_SYNC_HUNT 0x10u
#define RR0_CTS 0x20u
#define RR0_BREAK_ABORT 0x80u
#define RR0_TX_EMPTY 0x04u
#define RR1_FRAMING_ERROR 0x40u
#define RR1_CRC_ERROR 0x40u
#define RR1_END_OF_FRAME 0x80u
#define RR1_RX_OVERRUN 0x20u

#define WR5_RTS 0x02u
#define WR5_LED 0x80u
#define KISS_HW_SOFTWARE_DCD 1u
#define KISS_HW_HARDWARE_DCD 2u
#define KISS_HW_CTS 3u
#define A_WR5_DEFAULT 0xE9u
#define B_WR5_DEFAULT 0xEAu

#define BUFFER_DATA_SIZE 124u
#define BUFFER_COUNT 244u
#define QUEUE_SIZE 255u
#define INVALID_BUFFER 0xFFFFu

typedef uint16_t BufferRef;

typedef struct {
    BufferRef next;
    uint8_t nbytes;
    uint8_t nread;
    uint8_t data[BUFFER_DATA_SIZE];
} Buffer;

typedef enum {
    TX_IDLE = 0,
    TX_SLOT_WAIT = 1,
    TX_DELAY = 2,
    TX_CTS_WAIT = 3,
    TX_SENDING = 4,
    TX_TAIL = 5
} TxState;

typedef enum {
    KISS_HUNT = 0,
    KISS_COMMAND = 1,
    KISS_DATA = 2,
    KISS_ESCAPE = 3,
    KISS_TXDELAY = 10,
    KISS_PERSISTENCE = 20,
    KISS_SLOTTIME = 30,
    KISS_TXTAIL = 40,
    KISS_FULLDUPLEX = 50,
    KISS_HARDWARE = 60,
    KISS_HARDWARE_VALUE = 61
} KissState;

typedef struct {
    uint8_t txdelay;
    uint8_t persistence;
    uint8_t slottime;
    uint8_t txtail;
    uint8_t full_duplex;
    uint8_t cts_control;
    uint8_t software_dcd;
    uint8_t hardware_dcd;

    volatile uint8_t tx_state;
    volatile uint8_t tx_timer;
    volatile uint8_t tx_started;
    volatile uint8_t tx_outstanding;
    volatile uint8_t a_rr0;
    volatile uint8_t a_wr5;
    volatile uint8_t b_wr5;

    uint8_t rx_allocated;
    uint8_t rx_flushing;
    volatile uint8_t rx_state;
    BufferRef rx_head;
    BufferRef rx_current;

    uint8_t kiss_state;
    uint8_t in_allocated;
    BufferRef in_head;
    BufferRef in_current;
    uint8_t hardware_feature;

    volatile uint8_t host_out_started;
    volatile uint8_t host_escape;
    volatile uint8_t host_escape_byte;
    volatile BufferRef host_chain;
    volatile BufferRef tx_chain;

    volatile uint8_t tx_head;
    volatile uint8_t tx_tail;
    volatile uint8_t out_head;
    volatile uint8_t out_tail;
    BufferRef free_head;

    uint8_t host_break;
    uint8_t tick_divider;
    uint8_t tick_level;
} FirmwareState;

_Static_assert(sizeof(Buffer) == 128u, "Buffer nodes must remain 128 bytes");
_Static_assert(sizeof(FirmwareState) <= 0x50u,
               "Firmware state exceeds its fixed RAM reservation");

#ifdef HOST_TEST
extern volatile FirmwareState g_state;
extern Buffer g_buffers[BUFFER_COUNT];
extern volatile BufferRef g_tx_queue[QUEUE_SIZE];
extern volatile BufferRef g_out_queue[QUEUE_SIZE];
#else
extern volatile FirmwareState __at (0x8000) g_state;
extern volatile BufferRef __at (0x8100) g_tx_queue[QUEUE_SIZE];
extern volatile BufferRef __at (0x8300) g_out_queue[QUEUE_SIZE];
extern Buffer __at (0x8500) g_buffers[BUFFER_COUNT];
#endif

void state_reset(void);

void buffers_init(void);
BufferRef buffer_alloc(void);
void buffer_free(BufferRef ref);
void buffer_free_chain(BufferRef ref);
bool buffer_put(BufferRef *current, uint8_t byte);
bool buffer_get(BufferRef *current, uint8_t *byte);
uint16_t buffer_length(BufferRef ref);
bool tx_queue_push(BufferRef ref);
bool tx_queue_pop(BufferRef *ref);
bool out_queue_push(BufferRef ref);
bool out_queue_pop(BufferRef *ref);

void kiss_reset(void);
void kiss_receive_byte(uint8_t byte);
void kiss_abort_frame(void);

void modem_service(void);
void host_service(void);
void modem_receive_byte(uint8_t byte);
void modem_end_frame(uint8_t rr1);

void isr_b_tx(void);
void isr_b_ext(void);
void isr_b_rx(void);
void isr_b_special(void);
void isr_a_tx(void);
void isr_a_ext(void);
void isr_a_rx(void);
void isr_a_special(void);

void hardware_init(void);
void hardware_set_im2(void);
void hardware_irq_disable(void);
void hardware_irq_enable(void);
uint8_t hardware_random(void);
uint8_t hardware_a_ctrl_read(void);
uint8_t hardware_b_ctrl_read(void);
uint8_t hardware_a_data_read(void);
uint8_t hardware_b_data_read(void);
void hardware_a_ctrl_write(uint8_t value);
void hardware_b_ctrl_write(uint8_t value);
void hardware_a_data_write(uint8_t value);
void hardware_b_data_write(uint8_t value);
void hardware_sta(bool on);
void hardware_con(bool on);
void hardware_ptt(bool on);

void firmware_boot(void);

#endif
