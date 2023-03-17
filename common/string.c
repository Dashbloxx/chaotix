#include "string.h"
#include <stdalign.h>
#include <stdbool.h>
#include <stdint.h>

void* memset(void* s, int c, size_t n) {
    uintptr_t dest = (uintptr_t)s;

    bool aligned = !((uintptr_t)s & (alignof(uint32_t) - 1));
    if (aligned && n >= sizeof(uint32_t)) {
        size_t nd = n / sizeof(uint32_t);
        uint32_t d =
            (uint32_t)c << 24 | (uint32_t)c << 16 | (uint32_t)c << 8 | c;
        __asm__ volatile("rep stosl"
                         : "=D"(dest)
                         : "D"(dest), "c"(nd), "a"(d)
                         : "memory");
        n -= sizeof(uint32_t) * nd;
        if (n == 0)
            return s;
    }

    __asm__ volatile("rep stosb"
                     : "=D"(dest), "=c"(n)
                     : "0"(dest), "1"(n), "a"(c)
                     : "memory");
    return s;
}

void* memcpy(void* dest_ptr, const void* src_ptr, size_t n) {
    size_t dest = (size_t)dest_ptr;
    size_t src = (size_t)src_ptr;

    bool aligned = !((uintptr_t)dest_ptr & (alignof(uint32_t) - 1)) &&
                   !((uintptr_t)src_ptr & (alignof(uint32_t) - 1));
    if (aligned && n >= sizeof(uint32_t)) {
        size_t nd = n / sizeof(uint32_t);
        __asm__ volatile("rep movsl"
                         : "=S"(src), "=D"(dest)
                         : "S"(src), "D"(dest), "c"(nd)
                         : "memory");
        n -= sizeof(uint32_t) * nd;
        if (n == 0)
            return dest_ptr;
    }

    __asm__ volatile("rep movsb" ::"S"(src), "D"(dest), "c"(n) : "memory");
    return dest_ptr;
}

void* memmove(void* dest_ptr, const void* src_ptr, size_t n) {
    if (((uintptr_t)dest_ptr - (uintptr_t)src_ptr) >= n)
        return memcpy(dest_ptr, src_ptr, n);

    unsigned char* dest = (unsigned char*)dest_ptr + n;
    const unsigned char* src = (const unsigned char*)src_ptr + n;
    for (; n--;)
        *--dest = *--src;
    return dest_ptr;
}

int memcmp(const void* s1, const void* s2, size_t n) {
    const unsigned char* c1 = (const unsigned char*)s1;
    const unsigned char* c2 = (const unsigned char*)s2;
    while (n--) {
        if (*c1++ != *c2++)
            return c1[-1] < c2[-1] ? -1 : 1;
    }
    return 0;
}

int strcmp(const char* s1, const char* s2) {
    for (; *s1 == *s2; ++s1, ++s2) {
        if (*s1 == 0)
            return 0;
    }
    return *s1 < *s2 ? -1 : 1;
}

int strncmp(const char* s1, const char* s2, size_t n) {
    if (!n)
        return 0;
    do {
        if (*s1 != *s2++)
            return *(const unsigned char*)s1 - *(const unsigned char*)--s2;
        if (*s1++ == 0)
            break;
    } while (--n);
    return 0;
}

size_t strlen(const char* str) {
    size_t len = 0;
    while (*str++)
        ++len;
    return len;
}

size_t strnlen(const char* str, size_t n) {
    size_t len = 0;
    while (len < n && *str++)
        ++len;
    return len;
}

char* strcpy(char* dest, const char* src) {
    char* d = dest;
    while (*src)
        *d++ = *src++;
    return dest;
}

char* strncpy(char* dest, const char* src, size_t n) {
    size_t i;
    for (i = 0; i < n && src[i] != '\0'; ++i)
        dest[i] = src[i];
    for (; i < n; ++i)
        dest[i] = '\0';
    return dest;
}

size_t strlcpy(char* dst, const char* src, size_t size) {
    size_t len;
    for (len = 0; (len + 1 < size) && *src; len++)
        *dst++ = *src++;
    if (size > 0)
        *dst = '\0';
    while (*src++)
        len++;
    return len;
}

char* strcat(char* dest, const char* src) {
    size_t dest_len = strlen(dest);
    size_t i = 0;
    for (; src[i]; ++i)
        dest[dest_len + i] = src[i];
    dest[dest_len + i] = 0;
    return dest;
}

char* strncat(char* dest, const char* src, size_t n) {
    size_t dest_len = strlen(dest);
    size_t i = 0;
    for (; i < n && src[i]; ++i)
        dest[dest_len + i] = src[i];
    dest[dest_len + i] = 0;
    return dest;
}

char* strchr(const char* str, int ch) {
    while (*str) {
        if (*str == ch)
            return (char*)str;
        ++str;
    }
    return NULL;
}

char* strrchr(char const* str, int ch) {
    char* last = NULL;
    while (*str) {
        if (*str == ch)
            last = (char*)str;
        ++str;
    }
    return last;
}

char* strtok(char* str, char const* sep) {
    static char* saved_ptr;
    return strtok_r(str, sep, &saved_ptr);
}

char* strtok_r(char* str, const char* sep, char** last) {
    if (!str) {
        if (!*last)
            return NULL;
        str = *last;
    }

    while (*str) {
        if (!strchr(sep, *str))
            break;
        ++str;
    }
    if (!*str) {
        *last = NULL;
        return NULL;
    }

    char* end = str;
    while (*end) {
        if (strchr(sep, *end))
            break;
        ++end;
    }
    if (*end)
        *last = end + 1;
    else
        *last = NULL;
    *end = '\0';
    return str;
}

char* strstr(const char* str, const char* substr) {
    while (*str) {
        if (!strcmp(str, substr))
            return (char*)str;
        ++str;
    }
    return NULL;
}
