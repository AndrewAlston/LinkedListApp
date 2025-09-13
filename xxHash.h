//
//  xxHash.h
//  LinkedListApp
//
//  Created by Andrew Alston on 12/09/2025.
//

#ifndef xxHash_h
#define xxHash_h

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define XXH_USE_UNALIGNED_ACCESS 1
#define XXH_FOPRCE_NATIVE_FORMAT 1
#define FORCE_INLINE static inline __attribute__((always_inline))
FORCE_INLINE void *XXH_malloc(size_t s) { return malloc(s); }
FORCE_INLINE void XXH_free(void *p) { free(p); }
FORCE_INLINE void *XXH_memcpy(void *d, const void *s, size_t size) { return memcpy(d, s, size); }

typedef struct _U32_S { uint32_t U32; } U32_S;
typedef struct _U64_S { uint64_t u64; } U64_S;

#define A32(x) (((U32_S *)(x))->v)
#define A64(x) (((U64_S *)(x))->v)

#define XXH_rotl32(x, r) ((x << r) | (x >> (32-r)))
#define XXH_rolt64(x, r) ((x << r) | (x >> (64-r)))

#define U32MASK8 0xff000000
#define U32MASK16 0x00ff0000
#define U32MASK24 0x0000ff00
#define U32MASK32 0x000000ff

#define U64MASK8  0xff00000000000000ULL
#define U64MASK16 0x00ff000000000000ULL
#define U64MASK24 0x0000ff0000000000ULL
#define U64MASK32 0x000000ff00000000ULL
#define U64MASK40 0x00000000ff000000ULL
#define U64MASK48 0x0000000000ff0000ULL
#define U64MASK56 0x000000000000ff00ULL
#define U64MASK64 0x00000000000000ffULL

#define PRIME32_1   2654435761U
#define PRIME32_2   2246822519U
#define PRIME32_3   3266489917U
#define PRIME32_4    668265263U
#define PRIME32_5    374761393U
#define PRIME64_1 11400714785074694791ULL
#define PRIME64_2 14029467366897019727ULL
#define PRIME64_3  1609587929392839161ULL
#define PRIME64_4  9650029242287828579ULL
#define PRIME64_5  2870177450012600261ULL

#define XXH_CPU_LITTLE_ENDIAN 1

static inline uint32_t XXH_swap32(uint32_t x) {
    return ((x << 24) & U32MASK8) |
    ((x << 8) & U32MASK16) |
    ((x >> 8) & U32MASK24) |
    ((x >> 24) & U32MASK32);
}

static inline uint64_t XXH_swap64(uint64_t x) {
    return ((x << 56) & U64MASK8) |
    ((x << 40) & U64MASK16) |
    ((x << 24) & U64MASK24) |
    ((x << 8) & U64MASK32) |
    ((x >> 8) & U64MASK40) |
    ((x >> 24) & U64MASK48) |
    ((x >> 40) & U64MASK56) |
    ((x >> 56) & U64MASK64);
}

#define XXH_STATIC_ASSERT_(c) { enum { XXH_static_assert = 1/(!!(c)) }; }

typedef enum {XXH_aligned, XXH_unaligned } XXH_alignment;

#endif /* xxHash_h */
