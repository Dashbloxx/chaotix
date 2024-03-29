/*
 *  .OOOOOO.   OOOO                                .    O8O              
 *  D8P'  `Y8B  `888                              .O8    `"'              
 * 888           888 .OO.    .OOOO.    .OOOOO.  .O888OO OOOO  OOOO    OOO 
 * 888           888P"Y88B  `P  )88B  D88' `88B   888   `888   `88B..8P'  
 * 888           888   888   .OP"888  888   888   888    888     Y888'    
 * `88B    OOO   888   888  D8(  888  888   888   888 .  888   .O8"'88B   
 *  `Y8BOOD8P'  O888O O888O `Y888""8O `Y8BOD8P'   "888" O888O O88'   888O 
 * 
 *  Chaotix is a UNIX-like operating system that consists of a kernel written in C and
 *  i?86 assembly, and userland binaries written in C.
 *     
 *  Copyright (c) 2023 Nexuss
 *  Copyright (c) 2022 mosm
 *  Copyright (c) 2006-2018 Frans Kaashoek, Robert Morris, Russ Cox, Massachusetts Institute of Technology
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
#include "stdlib.h"

FILE *stdin = &(FILE) { .fd = 0 };
FILE *stdout = &(FILE) { .fd = 1 };
FILE *stderr = &(FILE) { .fd = 2 };

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

/* Close a file by it's `FILE` struct... */
int fclose(FILE *file) {
    return close(file->fd);
}

/* Write to a file by it's `FILE` struct... */
unsigned int fwrite(const void *ptr, unsigned int size, unsigned int count, FILE *stream) {
    unsigned int total_bytes_written = 0;

    /* Calculate the amount of bytes that are to be written... */
    unsigned int num_bytes_to_write = size * count;

    int bytes_written = write(stream->fd, ptr, num_bytes_to_write);

    /* Return an error of the amount if bytes written are less than zero... */
    if (bytes_written < 0) {
        return -1;
    }

    /* Update and return the amount of bytes that were written... */
    total_bytes_written += bytes_written;
    return total_bytes_written;
}

/* Read from file using `FILE` type... */
unsigned int fread(void *ptr, unsigned int size, unsigned int count, FILE *stream) {
    /* Calculate the amount of total bytes by multiplying the size of items by count of items... */
    unsigned int total_bytes = size * count;
    unsigned int bytes_read = 0;
    char *buffer = (char *) ptr;

    /* Get unique number that represents file... */
    int fd = stream->fd;

    while (bytes_read < total_bytes) {
        unsigned int remaining_bytes = total_bytes - bytes_read;
        int result = read(fd, buffer + bytes_read, remaining_bytes);

        if (result < 0) {
            break;
        }

        bytes_read += result;

        if (result == 0) {
            break;
        }
    }

    return bytes_read / size;
}