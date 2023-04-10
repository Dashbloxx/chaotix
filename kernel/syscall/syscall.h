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

#include <kernel/api/sys/socket.h>
#include <kernel/api/sys/stat.h>
#include <kernel/api/sys/syscall.h>
#include <kernel/api/sys/times.h>
#include <kernel/api/time.h>
#include <kernel/forward.h>
#include <stddef.h>
#include <stdnoreturn.h>

int sys_accept(int sockfd, struct sockaddr* addr, socklen_t* addrlen);
int sys_bind(int sockfd, const sockaddr* addr, socklen_t addrlen);
int sys_chdir(const char* path);
int sys_clock_gettime(clockid_t clk_id, struct timespec* tp);
int sys_clock_nanosleep(clockid_t clockid, int flags,
                        const struct timespec* request,
                        struct timespec* remain);
int sys_close(int fd);
int sys_connect(int sockfd, const struct sockaddr* addr, socklen_t addrlen);
int sys_dbgputs(const char* str);
int sys_dup2(int oldfd, int newfd);
int sys_execve(const char* pathname, char* const argv[], char* const envp[]);
noreturn void sys_exit(int status);
int sys_fcntl(int fd, int cmd, uintptr_t arg);
pid_t sys_fork(registers*);
int sys_ftruncate(int fd, off_t length);
char* sys_getcwd(char* buf, size_t size);
long sys_getdents(int fd, void* dirp, size_t count);
pid_t sys_getpgid(pid_t pid);
pid_t sys_getpid(void);
int sys_ioctl(int fd, int request, void* argp);
int sys_kill(pid_t pid, int sig);
int sys_link(const char* oldpath, const char* newpath);
int sys_listen(int sockfd, int backlog);
off_t sys_lseek(int fd, off_t offset, int whence);
int sys_mkdir(const char* pathname, mode_t mode);
int sys_mknod(const char* pathname, mode_t mode, dev_t dev);
void* sys_mmap(const mmap_params* params);
int sys_munmap(void* addr, size_t length);
int sys_open(const char* pathname, int flags, unsigned mode);
int sys_pipe(int pipefd[2]);
ssize_t sys_read(int fd, void* buf, size_t count);
int sys_reboot(int howto);
int sys_rename(const char* oldpath, const char* newpath);
int sys_rmdir(const char* pathname);
int sys_sched_yield(void);
int sys_setpgid(pid_t pid, pid_t pgid);
int sys_socket(int domain, int type, int protocol);
int sys_stat(const char* pathname, struct stat* buf);
long sys_sysconf(int name);
clock_t sys_times(struct tms* buf);
int sys_unlink(const char* pathname);
pid_t sys_waitpid(pid_t pid, int* wstatus, int options);
ssize_t sys_write(int fd, const void* buf, size_t count);
