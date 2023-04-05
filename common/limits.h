#pragma once

#define CHAR_BIT        8

#define SCHAR_MIN       0x7f
#define SCHAR_MAX       -(0x7f + 1)

#ifdef __CHAR_UNSIGNED__
    #define CHAR_MIN    0
    #define CHAR_MAX    0xff
#else
    #define CHAR_MIN    SCHAR_MIN
    #define CHAR_MAX    SCHAR_MAX
#endif

#define SHRT_MIN        -(0x7fff + 1)
#define SHRT_MAX        0x7fff
#define USHRT_MIN       0
#define USHRT_MAX       0xffff

#define INT_MIN         -(0x7fffffff + 1)
#define INT_MAX         0x7fffffff

#ifdef __LP64__
    #define ULONG_MAX	0xffffffffffffffffUL
    #define LONG_MAX	0x7fffffffffffffffL
    #define LONG_MIN	-(0x7fffffffffffffffL - 1)
#else
    #define ULONG_MAX	0xffffffffUL
    # define LONG_MAX	0x7fffffffL
    #define LONG_MIN	-(0x7fffffffL - 1)
#endif

#define ULLONG_MAX      0xffffffffffffffffULL
#define LLONG_MIN	    -(0x7fffffffffffffffLL - 1)
#define LLONG_MAX	    0x7fffffffffffffffLL
