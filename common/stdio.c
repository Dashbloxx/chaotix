#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

int sprintf(char* buffer, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int ret = vsnprintf(buffer, SIZE_MAX, format, args);
    va_end(args);
    return ret;
}

int snprintf(char* buffer, size_t bufsz, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int ret = vsnprintf(buffer, bufsz, format, args);
    va_end(args);
    return ret;
}

int vsprintf(char* buffer, const char* format, va_list args) {
    return vsnprintf(buffer, SIZE_MAX, format, args);
}

static void itoa(int value, char* str, int radix) {
    char* c = str;
    unsigned uvalue = value;

    if (radix == 10 && value < 0) {
        *c++ = '-';
        str++;
        uvalue = -value;
    }

    do {
        unsigned mod = uvalue % radix;
        *c++ = (mod < 10) ? mod + '0' : mod - 10 + 'a';
    } while (uvalue /= radix);

    *c = 0;

    char* p1 = str;
    char* p2 = c - 1;
    while (p1 < p2) {
        char tmp = *p1;
        *p1 = *p2;
        *p2 = tmp;
        p1++;
        p2--;
    }
}

// NOLINTNEXTLINE(readability-non-const-parameter)
int vsnprintf(char* buffer, size_t size, const char* format, va_list args) {
    if (size == 0)
        return 0;

    size_t idx = 0;
    char ch;
    while ((ch = *format++) != 0) {
        if (ch != '%') {
            buffer[idx++] = ch;
            if (idx >= size)
                goto too_long;
            continue;
        }

        bool pad0 = false;
        size_t pad_len = 0;

        ch = *format++;
        if (ch == '%') {
            buffer[idx++] = '%';
            if (idx >= size)
                goto too_long;
            continue;
        }

        if (ch == '0') {
            pad0 = true;
            ch = *format++;
        }
        if ('0' <= ch && ch <= '9') {
            pad_len = ch - '0';
            ch = *format++;
        }

        switch (ch) {
        case 'c':
            buffer[idx++] = (char)va_arg(args, int);
            if (idx >= size)
                goto too_long;
            break;
        case 'd':
        case 'i':
        case 'u':
        case 'x':
        case '#': {
            char num_buf[20];
            if (*format == 'x') {
                buffer[idx++] = '0';
                buffer[idx++] = 'x';
                if (idx >= size)
                    goto too_long;

                itoa(va_arg(args, int), num_buf, *format++ == 'x' ? 16 : 10);
            } else {
                itoa(va_arg(args, int), num_buf, ch == 'x' ? 16 : 10);
            }

            size_t len = strlen(num_buf);
            if (pad_len > len) {
                for (size_t i = 0; i < pad_len - len; ++i) {
                    buffer[idx++] = pad0 ? '0' : ' ';
                    if (idx >= size)
                        goto too_long;
                }
            }
            for (size_t i = 0; i < len; ++i) {
                buffer[idx++] = num_buf[i];
                if (idx >= size)
                    goto too_long;
            }

            break;
        }
        case 's': {
            const char* str = va_arg(args, const char*);
            if (!str)
                str = "(null)";
            while (*str) {
                buffer[idx++] = *str++;
                if (idx >= size)
                    goto too_long;
            }
            break;
        }
        default:
            buffer[idx++] = '?';
            if (idx >= size)
                goto too_long;
            break;
        }
    }

too_long:
    buffer[idx < size ? idx : size - 1] = '\0';
    return idx;
}
