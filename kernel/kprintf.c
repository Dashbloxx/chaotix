#include "kprintf.h"
#include "serial.h"
#include <stdarg.h>
#include <common/stdio.h>
#include <common/string.h>

/*
 *  Print a string as the kernel. For now, the kernel will just print
 *  out to COM1, although this could change...
 */
int kputs(const char* str) {
    return serial_write(SERIAL_COM1, str, strlen(str));
}

/*
 *  Do the same as above, but we can print out a formatted string that
 *  can contain other values which are passed. This is identical to
 *  `printf`, which is used frequently in most programs written in C.
 */
int kprintf(const char* format, ...) {
    char buf[1024];
    va_list args;
    va_start(args, format);
    int ret = vsnprintf(buf, 1024, format, args);
    va_end(args);
    kputs(buf);
    return ret;
}
