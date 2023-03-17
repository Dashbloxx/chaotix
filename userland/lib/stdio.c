#include "stdio.h"
#include "errno.h"
#include "string.h"
#include "unistd.h"

int putchar(int ch) {
    char c = ch;
    if (write(STDOUT_FILENO, &c, 1) < 0)
        return -1;
    return ch;
}

int puts(const char* str) {
    int rc = write(STDOUT_FILENO, str, strlen(str));
    if (rc < 0)
        return -1;
    if (write(STDOUT_FILENO, "\n", 1) < 0)
        return -1;
    return rc + 1;
}

int printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    int ret = vprintf(format, args);
    va_end(args);
    return ret;
}

int vprintf(const char* format, va_list ap) {
    return vdprintf(STDOUT_FILENO, format, ap);
}

int dprintf(int fd, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int ret = vdprintf(fd, format, args);
    va_end(args);
    return ret;
}

int vdprintf(int fd, const char* format, va_list ap) {
    char buf[1024];
    int len = vsnprintf(buf, 1024, format, ap);
    return write(fd, buf, len);
}

void perror(const char* s) {
    dprintf(STDERR_FILENO, "%s: %s\n", s, strerror(errno));
}

int getchar(void) {
    char c;
    if (read(STDIN_FILENO, &c, 1) < 0)
        return -1;
    return c;
}

int remove(const char* pathname) {
    if (unlink(pathname) >= 0)
        return 0;
    if (errno == EISDIR)
        return rmdir(pathname);
    return -1;
}
