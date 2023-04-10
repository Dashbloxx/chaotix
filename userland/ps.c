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

#include <dirent.h>
#include <errno.h>
#include <extra.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void) {
    DIR* dirp = opendir("/proc");
    if (!dirp) {
        perror("opendir");
        return EXIT_FAILURE;
    }

    printf("  PID CMD\n");
    for (;;) {
        errno = 0;
        struct dirent* dent = readdir(dirp);
        if (!dent) {
            if (errno == 0)
                break;
            perror("readdir");
            return EXIT_FAILURE;
        }

        if (!str_is_uint(dent->d_name))
            continue;

        pid_t pid = atoi(dent->d_name);

        char pathname[32];
        (void)snprintf(pathname, sizeof(pathname), "/proc/%d/comm", pid);
        int fd = open(pathname, O_RDONLY);
        if (fd < 0) {
            perror("open");
            return EXIT_FAILURE;
        }
        char comm[32];
        ssize_t nread = read(fd, comm, sizeof(comm));
        close(fd);

        if (nread < 0) {
            perror("read");
            return EXIT_FAILURE;
        }
        if (nread == 0)
            continue;

        if (comm[nread - 1] == '\n')
            comm[nread - 1] = 0;

        printf("%5d %s\n", pid, comm);
    }

    closedir(dirp);
    return EXIT_SUCCESS;
}
