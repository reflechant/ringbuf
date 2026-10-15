/********************************/
/* High throughput ring buffer */
/********************************/

#ifndef FAST_RINGBUF
#define FAST_RINGBUF

#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>

#ifndef FRB_DEF
#define FRB_DEF extern
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct frb_ringbuf {
  size_t cap;

  uint8_t *_base;
  uint8_t *_head;
  uint8_t *_tail;
};

FRB_DEF struct frb_ringbuf frb_new_ringbuf(size_t cap, void *buf);

FRB_DEF size_t frb_nfree(struct frb_ringbuf *buf);
FRB_DEF bool frb_is_empty(struct frb_ringbuf *buf);

FRB_DEF void *simd_memcpy_128(void *restrict dst, const void *restrict src,
                              size_t size);
FRB_DEF void *simd_memcpy_256(void *restrict dst, const void *restrict src,
                              size_t size);
FRB_DEF void *simd_memcpy_512(void *restrict dst, const void *restrict src,
                              size_t size);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // FAST_RINGFUB declarations

#ifdef FRB_IMPLEMENTATION

#include <immintrin.h>
#include <stdio.h>

#ifndef FRB_ASSERT
#include <assert.h>
#define FRB_ASSERT assert
#endif

// Alternative realization of memcpy using SIMD instructions.  It's a
// naive realization which works best for small chunks (up to ~1500
// byte - maximum size of standard IEEE 802.3 frame payload). For
// bigger chunks search for `apex_kryptonite` which uses prefetch and
// other tricks.
// Performance note: depending on the number of
// hardware AVX-512 blocks using AVX-256 may be faster, especially if
// you call these functions in parallel.

void *simd_memcpy_128(void *restrict dst, const void *restrict src,
                      size_t size) {
  size_t i = 0;
  for (; i < size; i += 16) {
    const __m128i c = _mm_loadu_si128((__m128i *)(src + i));
    _mm_storeu_si128((__m128i *)(dst + i), c);
  }
  for (; i < size; i += 8) {
    const uint64_t c = *((uint64_t *)(src + i));
    *((uint64_t *)(dst + i)) = c;
  }
  for (; i < size; i += 4) {
    const uint32_t c = *((uint32_t *)(src + i));
    *((uint32_t *)(dst + i)) = c;
  }
  for (; i < size; i++) {
    const uint8_t c = *((uint8_t *)(src + i));
    *((uint8_t *)(dst + i)) = c;
  }

  return dst;
}

void *simd_memcpy_256(void *restrict dst, const void *restrict src,
                      size_t size) {
  size_t i = 0;
  for (; i < size; i += 32) {
    const __m256i c = _mm256_loadu_si256((__m256i_u *)(src + i));
    _mm256_storeu_si256((__m256i_u *)(dst + i), c);
  }
  simd_memcpy_128(dst + i, src + i, size - i);

  return dst;
}

void *simd_memcpy_512(void *restrict dst, const void *restrict src,
                      size_t size) {
  size_t i = 0;
  for (; i < size; i += 64) {
    const __m512i c = _mm512_loadu_si512(src + i);
    _mm512_storeu_si512(dst + i, c);
  }
  simd_memcpy_256(dst + i, src + i, size - i);

  return dst;
}

bool frb_is_empty(struct frb_ringbuf *buf) { return buf->_tail == buf->_head; }

size_t nfilled(struct frb_ringbuf *buf) {
  if (buf->_head >= buf->_tail) {
    return buf->_head - buf->_tail;
  } else {
    return buf->cap - (buf->_tail - buf->_head);
  }
}

size_t frb_nfree(struct frb_ringbuf *buf) { return buf->cap - nfilled(buf); }

/* size_t headtoend(struct frb_ringbuf *buf) { */
/*   return buf->cap - (buf->_head - buf->_base); */
/* } */

/* size_t tailtoend(struct frb_ringbuf *buf) { */
/*   return buf->cap - (buf->_tail - buf->_base); */
/* } */

/* uint8_t *next_head(struct frb_ringbuf *buf, size_t offset) { */
/*   return buf->_base + ((buf->_head - buf->_base) + offset) % buf->cap; */
/* } */

/* uint8_t *next_tail(struct frb_ringbuf *buf, size_t offset) { */
/*   return buf->_base + ((buf->_tail - buf->_base) + offset) % buf->cap; */
/* } */

struct frb_ringbuf frb_new_ring(size_t size, void *buf) {
  FRB_ASSERT(size >= 0);
  FRB_ASSERT(buf != NULL);

  struct frb_ringbuf b;
  b._base = buf;
  b._head = b._base;
  b._tail = b._base;
  b.cap = size;
  return b;
}

#endif // FRB_IMPLEMENTATION

/********/
/* TODO */
/********/
// - Dive into performance profiling and optimization
// - Support ARM SIMD load/store, start with Apple Silicon
