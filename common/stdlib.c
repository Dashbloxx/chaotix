#include "stdlib.h"
#include "ctype.h"

int atoi(const char* str) {
    if (!*str)
        return 0;
    while (*str && isspace(*str))
        ++str;
    int res = 0;
    while ('0' <= *str && *str <= '9') {
        res *= 10;
        res += *str++ - '0';
    }
    return res;
}

int abs(int i) { return i < 0 ? -i : i; }

static void memswap(void* s1, void* s2, size_t n) {
    unsigned char* c1 = (unsigned char*)s1;
    unsigned char* c2 = (unsigned char*)s2;
    for (size_t i = 0; i < n; ++i) {
        unsigned char tmp = c1[i];
        c1[i] = c2[i];
        c2[i] = tmp;
    }
}

void qsort(void* base, size_t nmemb, size_t size,
           int (*compar)(const void*, const void*)) {
    unsigned char* start = base;
    while (nmemb > 1) {
        size_t pivot_point = nmemb / 2;
        if (pivot_point)
            memswap(start + pivot_point * size, start, size);

        const unsigned char* pivot = start;

        size_t i = 1;
        for (size_t j = 1; j < nmemb; ++j) {
            if (compar(start + j * size, pivot) < 0) {
                memswap(start + j * size, start + i * size, size);
                ++i;
            }
        }

        memswap(start, start + (i - 1) * size, size);
        if (i > nmemb / 2) {
            qsort(start + i * size, nmemb - i, size, compar);
            nmemb = i - 1;
        } else {
            qsort(start, i - 1, size, compar);
            nmemb -= i;
            start += i * size;
        }
    }
}
