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

#include "types.h"
#include <stddef.h>

#define SYSCALL_VECTOR 0x81

#define ENUMERATE_SYSCALLS(F)                                                  \
    F(accept)                                                                  \
    F(bind)                                                                    \
    F(chdir)                                                                   \
    F(clock_gettime)                                                           \
    F(clock_nanosleep)                                                         \
    F(close)                                                                   \
    F(connect)                                                                 \
    F(dbgputs)                                                                 \
    F(dup2)                                                                    \
    F(execve)                                                                  \
    F(exit)                                                                    \
    F(fcntl)                                                                   \
    F(fork)                                                                    \
    F(ftruncate)                                                               \
    F(getcwd)                                                                  \
    F(getdents)                                                                \
    F(getpgid)                                                                 \
    F(getpid)                                                                  \
    F(ioctl)                                                                   \
    F(kill)                                                                    \
    F(link)                                                                    \
    F(listen)                                                                  \
    F(lseek)                                                                   \
    F(mkdir)                                                                   \
    F(mknod)                                                                   \
    F(mmap)                                                                    \
    F(munmap)                                                                  \
    F(open)                                                                    \
    F(pipe)                                                                    \
    F(read)                                                                    \
    F(reboot)                                                                  \
    F(rename)                                                                  \
    F(rmdir)                                                                   \
    F(sched_yield)                                                             \
    F(setpgid)                                                                 \
    F(socket)                                                                  \
    F(stat)                                                                    \
    F(sysconf)                                                                 \
    F(times)                                                                   \
    F(unlink)                                                                  \
    F(waitpid)                                                                 \
    F(write)

enum {
#define DEFINE_ITEM(name) SYS_##name,
    ENUMERATE_SYSCALLS(DEFINE_ITEM)
#undef DEFINE_ITEM
        NUM_SYSCALLS
};

typedef struct mmap_params {
    void* addr;
    size_t length;
    int prot;
    int flags;
    int fd;
    off_t offset;
} mmap_params;
