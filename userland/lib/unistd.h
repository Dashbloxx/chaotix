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

#include <kernel/api/sys/types.h>
#include <kernel/api/unistd.h>
#include <stddef.h>

extern char** environ;

pid_t getpid(void);
int setpgid(pid_t pid, pid_t pgid);
pid_t getpgid(pid_t pid);

pid_t fork(void);
int execve(const char* pathname, char* const argv[], char* const envp[]);
int execvpe(const char* file, char* const argv[], char* const envp[]);

int open(const char* pathname, int flags, ...);
int close(int fd);
ssize_t read(int fd, void* buf, size_t count);
ssize_t write(int fd, const void* buf, size_t count);
int ftruncate(int fd, off_t length);
off_t lseek(int fd, off_t offset, int whence);
int mknod(const char* pathname, mode_t mode, dev_t dev);
int link(const char* oldpath, const char* newpath);
int unlink(const char* pathname);
int rename(const char* oldpath, const char* newpath);
int rmdir(const char* pathname);

int dup(int oldfd);
int dup2(int oldfd, int newfd);
int pipe(int pipefd[2]);

char* getcwd(char* buf, size_t size);
int chdir(const char* path);

pid_t tcgetpgrp(int fd);
int tcsetpgrp(int fd, pid_t pgrp);

unsigned int sleep(unsigned int seconds);
int usleep(useconds_t usec);

int reboot(int howto);

long sysconf(int name);

uintptr_t syscall(uint32_t num, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3,
                  uintptr_t arg4);
