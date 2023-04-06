/*
 *    __  __                      
 *   |  \/  |__ _ __ _ _ __  __ _ 
 *   | |\/| / _` / _` | '  \/ _` |
 *   |_|  |_\__,_\__, |_|_|_\__,_|
 *               |___/        
 * 
 *  Magma is a UNIX-like operating system that consists of a kernel written in C and
 *  i?86 assembly, and userland binaries written in C.
 *     
 *  Copyright (c) 2023 Nexuss, John Paul Wohlscheid, rilysh, Milton612, and FueledByCocaine
 * 
 *  This file may or may not contain code from https://github.com/mosmeh/yagura, and/or
 *  https://github.com/mit-pdos/xv6-public. Both projects have the same license as this
 *  project, and the license can be seen below:
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *  
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *  
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 *  THE SOFTWARE.
 */

#include "stdio.h"
#include "errno.h"
#include "string.h"
#include "unistd.h"
#include "fcntl.h"

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

FILE *fopen(const char *filename, const char *mode) {
    if(strcmp(mode, "r")) {
        FILE *fp;
        int fd = open(filename, O_RDONLY, mode);
        /* Check if `open` returned an error (-1). If not, just set the file struct's `fd` to what `open` returned. */
        if(fd != -1) {
            fp->fd = fd;
        }
        else {
            return NULL;
        }
        /* Now, if everything went fine, we can return the file struct... */
        return fp;
    }
    else if(strcmp(mode, "w")) {
        FILE *fp;
        /* This time, we ask `open` to make the file write-only, and to create it if it doesn't exist... */
        int fd = open(filename, O_WRONLY | O_CREAT, mode);
        /* Check if `open` returned an error (-1). If not, just set the file struct's `fd` to what `open` returned. */
        if(fd != -1) {
            fp->fd = fd;
        }
        else {
            return NULL;
        }
        /* Now, if everything went fine, we can return the file struct... */
        return fp;
    }
    else if(strcmp(mode, "r+")) {
        FILE *fp;
        /* This time, we ask `open` to make the file write-only, and to create it if it doesn't exist... */
        int fd = open(filename, O_RDWR, mode);
        /* Check if `open` returned an error (-1). If not, just set the file struct's `fd` to what `open` returned. */
        if(fd != -1) {
            fp->fd = fd;
        }
        else {
            return NULL;
        }
        /* Now, if everything went fine, we can return the file struct... */
        return fp;
    }
    else if(strcmp(mode, "w+")) {
        FILE *fp;
        /* This time, we ask `open` to make the file write-only, and to create it if it doesn't exist... */
        int fd = open(filename, O_RDWR | O_CREAT, mode);
        /* Check if `open` returned an error (-1). If not, just set the file struct's `fd` to what `open` returned. */
        if(fd != -1) {
            fp->fd = fd;
        }
        else {
            return NULL;
        }
        /* Now, if everything went fine, we can return the file struct... */
        return fp;
    }
    else {
        return NULL;
    }
}
