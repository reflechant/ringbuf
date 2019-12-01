#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

struct ringbuf
{
    size_t cap;
    uint8_t *base;
    uint8_t *head;
    uint8_t *tail;
};

bool is_empty(struct ringbuf *buf)
{
    return buf->tail == buf->head;
}

size_t nfilled(struct ringbuf *buf)
{
    if (buf->head >= buf->tail)
    {
        return buf->head - buf->tail;
    }
    else
    {
        return buf->cap - (buf->tail - buf->head);
    }
}

size_t nfree(struct ringbuf *buf)
{
    return buf->cap - nfilled(buf);
}

size_t get_cap(struct ringbuf *buf)
{
    return buf->cap;
}

size_t headtoend(struct ringbuf *buf)
{
    return buf->cap - (buf->head - buf->base);
}

size_t tailtoend(struct ringbuf *buf)
{
    return buf->cap - (buf->tail - buf->base);
}

uint8_t *next_head(struct ringbuf *buf, size_t offset)
{
    return buf->base + ((buf->head - buf->base) + offset) % buf->cap;
}

uint8_t *next_tail(struct ringbuf *buf, size_t offset)
{
    return buf->base + ((buf->tail - buf->base) + offset) % buf->cap;
}

struct ringbuf new_ring_buffer(size_t cap)
{
    if (cap < 1)
    {
        fprintf(stderr, "ringbuf capacity must be > 0\n");
        exit(EXIT_FAILURE);
    }
    struct ringbuf buf;
    buf.base = malloc(cap);
    if (buf.base == NULL)
    {
        fprintf(stderr, "can't allocate %zu memory for ringbuf\n", cap);
        exit(EXIT_FAILURE);
    }
    buf.head = buf.base;
    buf.tail = buf.base;
    buf.cap = cap;
    return buf;
}