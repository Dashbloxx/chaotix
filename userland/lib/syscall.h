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

#pragma once

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
    unsigned int length;
    int prot;
    int flags;
    int fd;
    int offset;
} mmap_params;
