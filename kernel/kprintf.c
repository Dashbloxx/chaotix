#include "kprintf.h"
#include "serial.h"
#include <stdarg.h>
#include <common/stdio.h>
#include <common/string.h>
//#include <string.h>

int kputs(const char* str) {
    return serial_write(SERIAL_COM1, str, strlen(str));
}

int kprintf(const char* format, ...) {
    char buf[1024];
    va_list args;
    va_start(args, format);
    int ret = vsnprintf(buf, 1024, format, args);
    va_end(args);
    kputs(buf);
    return ret;
}
