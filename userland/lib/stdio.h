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

#pragma once

#include <common/stdio.h>
#include <kernel/api/stdio.h>

/*
 *  This struct contains all the values that you'll usually find in the `FILE` struct type. Well, the type is actually `file_t`, but
 *  then theres a macro that defines `FILE` as `file_t`...
 */
typedef struct {
    int fd; /* Contains a unique integer that represents the file. Usually this integer can be returned by the `open` syscall... */
    int mode; /* This contains the mode in which the file is opened... */
    int error; /* This is to contain an error code, if an error happens... */
    unsigned char *buffer; /* The buffer is stored in RAM, and is to have the file's contents replicated into this buffer... */
    unsigned int buffer_size; /* This contains the size that the buffer should be. */
    unsigned int position; /* The current position in file, whether it's being read or written to... */
} file_t;

/* Let's define `FILE` the right way. If we want it capitalized, let's use a macro to do it! */
#define FILE file_t

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

int putchar(int ch);
int puts(const char* str);
int printf(const char* format, ...);
int vprintf(const char* format, va_list ap);
int dprintf(int fd, const char* format, ...);
int vdprintf(int fd, const char* format, va_list ap);

void perror(const char*);

int getchar(void);

int remove(const char* pathname);

int dbgputs(const char* str);

FILE *fopen(const char *filename, const char *mode);
int fclose(FILE *file);
unsigned int fwrite(const void *ptr, unsigned int size, unsigned int count, FILE *stream);
unsigned int fread(void *ptr, unsigned int size, unsigned int count, FILE *stream);