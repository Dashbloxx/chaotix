#include "strings.h"
#include "ctype.h"

int strcasecmp(const char* s1, const char* s2) {
    for (; tolower(*s1) == tolower(*s2); ++s1, ++s2) {
        if (*s1 == 0)
            return 0;
    }
    return tolower(*s1) < tolower(*s2) ? -1 : 1;
}

int strncasecmp(const char* s1, const char* s2, size_t n) {
    if (!n)
        return 0;
    do {
        if (tolower(*s1) != tolower(*s2++))
            return tolower(*(const unsigned char*)s1) -
                   tolower(*(const unsigned char*)--s2);
        if (*s1++ == 0)
            break;
    } while (--n);
    return 0;
}
