#include "tnc2.h"

static uint8_t queue_next(uint8_t value)
{
    ++value;
    return value == QUEUE_SIZE ? 0 : value;
}

void buffers_init(void)
{
    uint16_t i;

    for (i = 0; i < BUFFER_COUNT; ++i) {
        g_buffers[i].next = (i + 1u < BUFFER_COUNT)
                                ? (BufferRef)(i + 1u)
                                : INVALID_BUFFER;
        g_buffers[i].nbytes = 0;
        g_buffers[i].nread = 0;
    }
    g_state.free_head = 0;
    g_state.tx_head = 0;
    g_state.tx_tail = 0;
    g_state.out_head = 0;
    g_state.out_tail = 0;
    g_state.tx_outstanding = 0;
}

BufferRef buffer_alloc(void)
{
    BufferRef ref = g_state.free_head;
    Buffer *buffer;

    if (ref == INVALID_BUFFER) {
        return INVALID_BUFFER;
    }
    buffer = &g_buffers[ref];
    g_state.free_head = buffer->next;
    buffer->next = INVALID_BUFFER;
    buffer->nbytes = 0;
    buffer->nread = 0;
    return ref;
}

void buffer_free(BufferRef ref)
{
    Buffer *buffer;

    if (ref == INVALID_BUFFER || ref >= BUFFER_COUNT) {
        return;
    }
    buffer = &g_buffers[ref];
    buffer->nbytes = 0;
    buffer->nread = 0;
    buffer->next = g_state.free_head;
    g_state.free_head = ref;
}

void buffer_free_chain(BufferRef ref)
{
    while (ref != INVALID_BUFFER && ref < BUFFER_COUNT) {
        BufferRef next = g_buffers[ref].next;
        buffer_free(ref);
        ref = next;
    }
}

bool buffer_put(BufferRef *current, uint8_t byte)
{
    Buffer *buffer;

    if (*current == INVALID_BUFFER || *current >= BUFFER_COUNT) {
        return false;
    }
    buffer = &g_buffers[*current];
    if (buffer->nbytes == BUFFER_DATA_SIZE) {
        BufferRef next = buffer_alloc();
        if (next == INVALID_BUFFER) {
            return false;
        }
        buffer->next = next;
        *current = next;
        buffer = &g_buffers[next];
    }
    buffer->data[buffer->nbytes++] = byte;
    return true;
}

bool buffer_get(BufferRef *current, uint8_t *byte)
{
    Buffer *buffer;

    while (*current != INVALID_BUFFER && *current < BUFFER_COUNT) {
        buffer = &g_buffers[*current];
        if (buffer->nread < buffer->nbytes) {
            if ((uint8_t)(buffer->nread + 1u) == buffer->nbytes &&
                buffer->next == INVALID_BUFFER) {
                BufferRef old = *current;
                *current = INVALID_BUFFER;
                buffer_free(old);
                return false;
            }
            *byte = buffer->data[buffer->nread++];
            return true;
        }
        {
            BufferRef old = *current;
            *current = buffer->next;
            buffer_free(old);
        }
    }
    return false;
}

uint16_t buffer_length(BufferRef ref)
{
    uint16_t length = 0;

    while (ref != INVALID_BUFFER && ref < BUFFER_COUNT) {
        length += g_buffers[ref].nbytes;
        ref = g_buffers[ref].next;
    }
    return length;
}

bool tx_queue_push(BufferRef ref)
{
    uint8_t next = queue_next(g_state.tx_tail);
    if (next == g_state.tx_head) {
        return false;
    }
    g_tx_queue[g_state.tx_tail] = ref;
    g_state.tx_tail = next;
    ++g_state.tx_outstanding;
    return true;
}

bool tx_queue_pop(BufferRef *ref)
{
    if (g_state.tx_head == g_state.tx_tail) {
        return false;
    }
    *ref = g_tx_queue[g_state.tx_head];
    g_state.tx_head = queue_next(g_state.tx_head);
    return true;
}

bool out_queue_push(BufferRef ref)
{
    uint8_t next = queue_next(g_state.out_tail);
    if (next == g_state.out_head) {
        return false;
    }
    g_out_queue[g_state.out_tail] = ref;
    g_state.out_tail = next;
    return true;
}

bool out_queue_pop(BufferRef *ref)
{
    if (g_state.out_head == g_state.out_tail) {
        return false;
    }
    *ref = g_out_queue[g_state.out_head];
    g_state.out_head = queue_next(g_state.out_head);
    return true;
}
