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

#include "unistd.h"
#include "errno.h"
#include "fcntl.h"
#include "panic.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "sys/ioctl.h"
#include "time.h"

char** environ;

int execvpe(const char* file, char* const argv[], char* const envp[]) {
    if (strchr(file, '/'))
        return execve(file, argv, envp);

    const char* path = getenv("PATH");
    if (!path)
        path = "/bin";
    char* dup_path = strdup(path);
    if (!dup_path)
        return -1;

    int saved_errno = errno;

    static const char* sep = ":";
    char* saved_ptr;
    for (const char* part = strtok_r(dup_path, sep, &saved_ptr); part;
         part = strtok_r(NULL, sep, &saved_ptr)) {
        static char buf[1024];
        ASSERT(sprintf(buf, "%s/%s", part, file) > 0);
        int rc = execve(buf, argv, envp);
        ASSERT(rc < 0);
        if (errno != ENOENT)
            return -1;
        errno = saved_errno;
    }

    errno = ENOENT;
    return -1;
}

int dup(int oldfd) { return fcntl(oldfd, F_DUPFD); }

unsigned int sleep(unsigned int seconds) {
    struct timespec req = {.tv_sec = seconds, .tv_nsec = 0};
    struct timespec rem;
    if (nanosleep(&req, &rem) < 0)
        return rem.tv_sec;
    return 0;
}

int usleep(useconds_t usec) {
    struct timespec req = {.tv_sec = usec / 1000000,
                           .tv_nsec = (usec % 1000000) * 1000};
    return nanosleep(&req, NULL);
}

pid_t tcgetpgrp(int fd) { return ioctl(fd, TIOCGPGRP, NULL); }

int tcsetpgrp(int fd, pid_t pgrp) { return ioctl(fd, TIOCSPGRP, &pgrp); }
