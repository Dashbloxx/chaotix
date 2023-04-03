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

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUF_SIZE 1024

static int dump_file(const char* filename) {
    int fd = strcmp(filename, "-") ? open(filename, O_RDONLY) : 0;
    if (fd < 0)
        return -1;
    for (;;) {
        static char buf[BUF_SIZE];
        ssize_t nread = read(fd, buf, BUF_SIZE);
        if (nread < 0) {
            if (fd > 0)
                close(fd);
            return -1;
        }
        if (nread == 0)
            break;
        if (write(STDOUT_FILENO, buf, nread) < 0)
            return -1;
    }
    if (fd > 0)
        close(fd);
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        if (dump_file("-") < 0) {
            perror("dump_file");
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }
    for (int i = 1; i < argc; ++i) {
        if (dump_file(argv[i]) < 0) {
            perror("dump_file");
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}
