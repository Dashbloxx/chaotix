#pragma once

#include "ctype.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define _STRINGIFY(x) #x
#define STRINGIFY(x) _STRINGIFY(x)

#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))

#define NODISCARD __attribute__((__warn_unused_result__))

static inline uintptr_t round_up(uintptr_t x, size_t align) {
    return (x + (align - 1)) & ~(align - 1);
}

static inline uintptr_t round_down(uintptr_t x, size_t align) {
    return x & ~(align - 1);
}

static inline size_t div_ceil(size_t lhs, size_t rhs) {
    return (lhs + rhs - 1) / rhs;
}

static inline size_t next_power_of_two(size_t x) {
    if (x <= 1)
        return 1;
    return (SIZE_MAX >> __builtin_clz(x - 1)) + 1;
}

// NOLINTNEXTLINE(readability-non-const-parameter)
static inline void memset32(uint32_t* dest, uint32_t c, size_t n) {
    __asm__ volatile("rep stosl"
                     : "=D"(dest), "=c"(n)
                     : "D"(dest), "c"(n), "a"(c)
                     : "memory");
}

// NOLINTNEXTLINE(readability-non-const-parameter)
static inline void memcpy32(uint32_t* dest, const uint32_t* src, size_t n) {
    __asm__ volatile("rep movsl" : "+S"(src), "+D"(dest), "+c"(n)::"memory");
}

static inline bool str_is_uint(const char* s) {
    while (*s) {
        if (!isdigit(*s))
            return false;
        ++s;
    }
    return true;
}
