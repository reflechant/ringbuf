#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <x86intrin.h>

struct ringbuf
{
    size_t cap;
    uint8_t *base;
    uint8_t *head;
    uint8_t *tail;
};

// Alternative realization of memcpy using SIMD instructions. Looks like 256-bit version is the fastest on Intel Xeon Silver CPUs even having AVX-512 is available.
// It's a naive realization which works best for small chunks (up to ~1500 byte - maximum size of standard IEEE 802.3 frame payload). For bigger chunks search for `apex_kryptonite` which uses prefetch and other tricks
void *intrin_memcpy_512(void *restrict dst, const void *restrict src, size_t size)
{
    size_t i = 0;
    for (; i < size; i += 64)
    {
        const __m512i c = _mm512_loadu_si512(src + i);
        _mm512_storeu_si512(dst + i, c);
    }
    intrin_memcpy_256(dst + i, src + i, size - i);

    return dst;
}

void *intrin_memcpy_256(void *restrict dst, const void *restrict src, size_t size)
{
    size_t i = 0;
    for (; i < size; i += 32)
    {
        const __m256i c = _mm256_loadu_si256((__m256i_u *)(src + i));
        _mm256_storeu_si256((__m256i_u *)(dst + i), c);
    }
    intrin_memcpy_128(dst + i, src + i, size - i);

    return dst;
}

void *intrin_memcpy_128(void *restrict dst, const void *restrict src, size_t size)
{
    size_t i = 0;
    for (; i < size; i += 16)
    {
        const __m128i c = _mm_loadu_si128((__m128i *)(src + i));
        _mm_storeu_si128((__m128i *)(dst + i), c);
    }
    for (; i < size; i += 8)
    {
        const uint64_t c = *((uint64_t *)(src + i));
        *((uint64_t *)(dst + i)) = c;
    }
    for (; i < size; i += 4)
    {
        const uint32_t c = *((uint32_t *)(src + i));
        *((uint32_t *)(dst + i)) = c;
    }
    for (; i < size; i++)
    {
        const uint8_t c = *((uint8_t *)(src + i));
        *((uint8_t *)(dst + i)) = c;
    }

    return dst;
}

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
        fprintf(stderr, "ringbuf capacity must be nonzero\n");
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
