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

#include "dirent.h"
#include "errno.h"
#include "stdlib.h"
#include "fcntl.h"
#include "unistd.h"

typedef struct DIR {
    int fd;
    unsigned char* buf;
    size_t buf_capacity, buf_size;
    size_t buf_cursor;
} DIR;

DIR* opendir(const char* name) {
    DIR* dirp = malloc(sizeof(DIR));
    if (!dirp)
        return NULL;
    *dirp = (DIR){0};
    dirp->fd = open(name, O_RDONLY);
    if (dirp->fd < 0)
        return NULL;
    dirp->buf_capacity = 1024;
    return dirp;
}

int closedir(DIR* dirp) {
    int rc = close(dirp->fd);
    free(dirp->buf);
    free(dirp);
    return rc;
}

struct dirent* readdir(DIR* dirp) {
    if (dirp->buf_cursor >= dirp->buf_size) {
        int saved_errno = errno;
        for (;;) {
            unsigned char* new_buf = realloc(dirp->buf, dirp->buf_capacity);
            if (!new_buf)
                return NULL;
            dirp->buf = new_buf;
            errno = 0;
            ssize_t nread = getdents(dirp->fd, dirp->buf, dirp->buf_capacity);
            if (nread == 0) {
                errno = saved_errno;
                return NULL;
            }
            if (nread > 0) {
                dirp->buf_size = nread;
                break;
            }
            if (errno != EINVAL)
                return NULL;
            dirp->buf_capacity *= 2;
        }
        errno = saved_errno;
        dirp->buf_cursor = 0;
    }
    struct dirent* dent = (struct dirent*)(dirp->buf + dirp->buf_cursor);
    dirp->buf_cursor += dent->d_reclen;
    return dent;
}
