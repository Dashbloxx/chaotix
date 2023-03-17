#pragma once

static inline int isprint(int c) { return (int)(0x20 <= c && c <= 0x7e); }

static inline int isspace(int c) {
    switch (c) {
    case '\t':
    case '\n':
    case '\v':
    case '\f':
    case '\r':
    case ' ':
        return 1;
    }
    return 0;
}

static inline int isgraph(int c) { return (int)(0x21 <= c && c <= 0x7e); }

static inline int isdigit(int c) { return (int)('0' <= c && c <= '9'); }

static inline int isalnum(int c) {
    if (isdigit(c))
        return 1;
    if ('A' <= c && c <= 'Z')
        return 1;
    return 'a' <= c && c <= 'z';
}

static inline int isxdigit(int c) {
    if (isdigit(c))
        return 1;
    if ('A' <= c && c <= 'F')
        return 1;
    return 'a' <= c && c <= 'f';
}

static inline int isascii(int c) { return (unsigned)c <= 127; }

static inline int tolower(int c) {
    if ('A' <= c && c <= 'Z')
        return c | 0x20;
    return c;
}

static inline int toupper(int c) {
    if ('a' <= c && c <= 'z')
        return c & ~0x20;
    return c;
}
