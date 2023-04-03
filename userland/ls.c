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

#include <dirent.h>
#include <errno.h>
#include <extra.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static int get_format(const struct dirent* dent, const char** out_format,
                      size_t* out_len) {
    struct stat stat_buf;
    if (stat(dent->d_name, &stat_buf) < 0)
        return -1;
    switch (stat_buf.st_mode & S_IFMT) {
    case S_IFDIR:
        *out_format = "\x1b[01;34m%s\x1b[m/";
        *out_len = strlen(dent->d_name) + 1;
        break;
    case S_IFCHR:
    case S_IFBLK:
        *out_format = "\x1b[01;33m%s\x1b[m";
        *out_len = strlen(dent->d_name);
        break;
    case S_IFIFO:
        *out_format = "\x1b[33m%s\x1b[m|";
        *out_len = strlen(dent->d_name) + 1;
        break;
    case S_IFLNK:
        *out_format = "\x1b[01;36m%s\x1b[m@";
        *out_len = strlen(dent->d_name) + 1;
        break;
    case S_IFSOCK:
        *out_format = "\x1b[01;35m%s\x1b[m=";
        *out_len = strlen(dent->d_name) + 1;
        break;
    case S_IFREG:
    default:
        if (stat_buf.st_mode & S_IXUSR) {
            *out_format = "\x1b[01;32m%s\x1b[m*";
            *out_len = strlen(dent->d_name) + 1;
        } else {
            *out_format = "%s";
            *out_len = strlen(dent->d_name);
        }
        break;
    }
    return 0;
}

#define TAB_STOP 8

int main(int argc, char* argv[]) {
    const char* path;
    if (argc < 2) {
        static char path_buf[1024];
        getcwd(path_buf, 1024);
        path = path_buf;
    } else if (argc == 2) {
        path = argv[1];
    } else {
        dprintf(STDERR_FILENO, "Usage: ls [DIRECTORY]\n");
        return EXIT_FAILURE;
    }

    DIR* dirp = opendir(path);
    if (!dirp) {
        perror("opendir");
        return EXIT_FAILURE;
    }
    chdir(path);

    size_t width = 0;
    for (;;) {
        errno = 0;
        struct dirent* dent = readdir(dirp);
        if (!dent) {
            if (errno == 0)
                break;
            perror("readdir");
            return EXIT_FAILURE;
        }

        const char* format;
        size_t len;
        if (get_format(dent, &format, &len) < 0) {
            perror("get_format");
            return EXIT_FAILURE;
        }
        size_t next_pos = round_up(width + len + 1, TAB_STOP);
        if (next_pos >= 80) {
            width = round_up(len + 1, TAB_STOP);
            putchar('\n');
        } else {
            width = next_pos;
        }
        printf(format, dent->d_name);
        putchar('\t');
    }
    if (width > 0)
        putchar('\n');

    closedir(dirp);
    return EXIT_SUCCESS;
}
