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

#include <err.h>
#include <errno.h>
#include <extra.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdnoreturn.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <time.h>

#include "syscall.h"

uintptr_t syscall(uint32_t num, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3,
                  uintptr_t arg4) {
    uintptr_t ret;
    __asm__ volatile("int $" STRINGIFY(SYSCALL_VECTOR)
                     : "=a"(ret)
                     : "a"(num), "d"(arg1), "c"(arg2), "b"(arg3), "S"(arg4)
                     : "memory");
    return ret;
}

#define RETURN_WITH_ERRNO(rc, type)                                            \
    do {                                                                       \
        if (IS_ERR(rc)) {                                                      \
            errno = -(rc);                                                     \
            return (type)-1;                                                   \
        }                                                                      \
        return (type)(rc);                                                     \
    } while (0);

int accept(int sockfd, struct sockaddr* addr, socklen_t* addrlen) {
    int rc =
        syscall(SYS_accept, sockfd, (uintptr_t)addr, (uintptr_t)addrlen, 0);
    RETURN_WITH_ERRNO(rc, int)
}

int bind(int sockfd, const sockaddr* addr, socklen_t addrlen) {
    int rc = syscall(SYS_bind, sockfd, (uintptr_t)addr, addrlen, 0);
    RETURN_WITH_ERRNO(rc, int)
}

int chdir(const char* path) {
    int rc = syscall(SYS_chdir, (uintptr_t)path, 0, 0, 0);
    RETURN_WITH_ERRNO(rc, int)
}

int clock_gettime(clockid_t clk_id, struct timespec* tp) {
    int rc = syscall(SYS_clock_gettime, clk_id, (uintptr_t)tp, 0, 0);
    RETURN_WITH_ERRNO(rc, int)
}

int clock_nanosleep(clockid_t clockid, int flags,
                    const struct timespec* request, struct timespec* remain) {
    int rc = syscall(SYS_clock_nanosleep, clockid, flags, (uintptr_t)request,
                     (uintptr_t)remain);
    // unlike other syscall wrappers, clock_nanosleep returns the error value
    // instead of returning -1 and setting errno
    if (IS_ERR(rc))
        return -rc;
    return 0;
}

int close(int fd) {
    int rc = syscall(SYS_close, fd, 0, 0, 0);
    RETURN_WITH_ERRNO(rc, int)
}

int connect(int sockfd, const struct sockaddr* addr, socklen_t addrlen) {
    int rc = syscall(SYS_connect, sockfd, (uintptr_t)addr, addrlen, 0);
    RETURN_WITH_ERRNO(rc, int)
}

int dbgputs(const char* str) {
    int rc = syscall(SYS_dbgputs, (uintptr_t)str, 0, 0, 0);
    RETURN_WITH_ERRNO(rc, int)
}

int dup2(int oldfd, int newfd) {
    int rc = syscall(SYS_dup2, oldfd, newfd, 0, 0);
    RETURN_WITH_ERRNO(rc, int)
}

int execve(const char* pathname, char* const argv[], char* const envp[]) {
    int rc = syscall(SYS_execve, (uintptr_t)pathname, (uintptr_t)argv,
                     (uintptr_t)envp, 0);
    RETURN_WITH_ERRNO(rc, int)
}

noreturn void exit(int status) {
    syscall(SYS_exit, status, 0, 0, 0);
    __builtin_unreachable();
}

int fcntl(int fd, int cmd, ...) {
    va_list args;
    va_start(args, cmd);
    int arg = va_arg(args, int);
    va_end(args);
    int rc = syscall(SYS_fcntl, fd, cmd, arg, 0);
    RETURN_WITH_ERRNO(rc, int)
}

pid_t fork(void) {
    int rc = syscall(SYS_fork, 0, 0, 0, 0);
    RETURN_WITH_ERRNO(rc, pid_t)
}

int ftruncate(int fd, off_t length) {
    int rc = syscall(SYS_ftruncate, fd, length, 0, 0);
    RETURN_WITH_ERRNO(rc, int)
}

char* getcwd(char* buf, size_t size) {
    int rc = syscall(SYS_getcwd, (uintptr_t)buf, size, 0, 0);
    if (IS_ERR(rc)) {
        errno = -rc;
        return NULL;
    }
    return (char*)rc;
}

long getdents(int fd, void* dirp, size_t count) {
    int rc = syscall(SYS_getdents, fd, (uintptr_t)dirp, count, 0);
    RETURN_WITH_ERRNO(rc, long)
}

pid_t getpgid(pid_t pid) {
    int rc = syscall(SYS_getpgid, pid, 0, 0, 0);
    RETURN_WITH_ERRNO(rc, pid_t)
}

pid_t getpid(void) {
    int rc = syscall(SYS_getpid, 0, 0, 0, 0);
    RETURN_WITH_ERRNO(rc, pid_t)
}

int ioctl(int fd, int request, void* argp) {
    int rc = syscall(SYS_ioctl, fd, request, (uintptr_t)argp, 0);
    RETURN_WITH_ERRNO(rc, int)
}

int kill(pid_t pid, int sig) {
    int rc = syscall(SYS_kill, pid, sig, 0, 0);
    RETURN_WITH_ERRNO(rc, int)
}

int link(const char* oldpath, const char* newpath) {
    int rc = syscall(SYS_link, (uintptr_t)oldpath, (uintptr_t)newpath, 0, 0);
    RETURN_WITH_ERRNO(rc, int)
}

int listen(int sockfd, int backlog) {
    int rc = syscall(SYS_listen, sockfd, backlog, 0, 0);
    RETURN_WITH_ERRNO(rc, int)
}

off_t lseek(int fd, off_t offset, int whence) {
    int rc = syscall(SYS_lseek, fd, offset, whence, 0);
    RETURN_WITH_ERRNO(rc, off_t)
}

int mkdir(const char* pathname, mode_t mode) {
    int rc = syscall(SYS_mkdir, (uintptr_t)pathname, mode, 0, 0);
    RETURN_WITH_ERRNO(rc, int)
}

int mknod(const char* pathname, mode_t mode, dev_t dev) {
    int rc = syscall(SYS_mknod, (uintptr_t)pathname, mode, dev, 0);
    RETURN_WITH_ERRNO(rc, int)
}

void* mmap(void* addr, size_t length, int prot, int flags, int fd,
           off_t offset) {
    mmap_params params;
    params.addr = addr;
    params.length = length;
    params.prot = prot;
    params.flags = flags;
    params.fd = fd;
    params.offset = offset;

    int rc = syscall(SYS_mmap, (uintptr_t)&params, 0, 0, 0);
    RETURN_WITH_ERRNO(rc, void*)
}

int munmap(void* addr, size_t length) {
    int rc = syscall(SYS_munmap, (uintptr_t)addr, length, 0, 0);
    RETURN_WITH_ERRNO(rc, int)
}

int open(const char* pathname, int flags, ...) {
    unsigned mode = 0;
    if (flags & O_CREAT) {
        va_list args;
        va_start(args, flags);
        mode = va_arg(args, unsigned);
        va_end(args);
    }
    int rc = syscall(SYS_open, (uintptr_t)pathname, flags, mode, 0);
    RETURN_WITH_ERRNO(rc, int)
}

int pipe(int pipefd[2]) {
    int rc = syscall(SYS_pipe, (uintptr_t)pipefd, 0, 0, 0);
    RETURN_WITH_ERRNO(rc, int)
}

ssize_t read(int fd, void* buf, size_t count) {
    int rc = syscall(SYS_read, fd, (uintptr_t)buf, count, 0);
    RETURN_WITH_ERRNO(rc, ssize_t)
}

int reboot(int howto) {
    int rc = syscall(SYS_reboot, howto, 0, 0, 0);
    RETURN_WITH_ERRNO(rc, int)
}

int rename(const char* oldpath, const char* newpath) {
    int rc = syscall(SYS_rename, (uintptr_t)oldpath, (uintptr_t)newpath, 0, 0);
    RETURN_WITH_ERRNO(rc, int)
}

int rmdir(const char* pathname) {
    int rc = syscall(SYS_rmdir, (uintptr_t)pathname, 0, 0, 0);
    RETURN_WITH_ERRNO(rc, int)
}

int sched_yield(void) {
    int rc = syscall(SYS_sched_yield, 0, 0, 0, 0);
    RETURN_WITH_ERRNO(rc, int)
}

int setpgid(pid_t pid, pid_t pgid) {
    int rc = syscall(SYS_setpgid, pid, pgid, 0, 0);
    RETURN_WITH_ERRNO(rc, int)
}

int socket(int domain, int type, int protocol) {
    int rc = syscall(SYS_socket, domain, type, protocol, 0);
    RETURN_WITH_ERRNO(rc, int)
}

int stat(const char* pathname, struct stat* buf) {
    int rc = syscall(SYS_stat, (uintptr_t)pathname, (uintptr_t)buf, 0, 0);
    RETURN_WITH_ERRNO(rc, int)
}

long sysconf(int name) {
    int rc = syscall(SYS_sysconf, name, 0, 0, 0);
    RETURN_WITH_ERRNO(rc, long)
}

clock_t times(struct tms* buf) {
    int rc = syscall(SYS_times, (uintptr_t)buf, 0, 0, 0);
    RETURN_WITH_ERRNO(rc, clock_t)
}

int unlink(const char* pathname) {
    int rc = syscall(SYS_unlink, (uintptr_t)pathname, 0, 0, 0);
    RETURN_WITH_ERRNO(rc, int)
}

pid_t waitpid(pid_t pid, int* wstatus, int options) {
    int rc = syscall(SYS_waitpid, pid, (uintptr_t)wstatus, options, 0);
    RETURN_WITH_ERRNO(rc, pid_t)
}

ssize_t write(int fd, const void* buf, size_t count) {
    int rc = syscall(SYS_write, fd, (uintptr_t)buf, count, 0);
    RETURN_WITH_ERRNO(rc, ssize_t)
}
