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

#include <errno.h>
#include <extra.h>
#include <fb.h>
#include <fcntl.h>
#include <panic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

static noreturn void shm_reader(void) {
    int fd = open("/dev/shm/test-fs", O_RDWR);
    ASSERT_OK(fd);
    size_t size = 50000 * sizeof(int);
    ASSERT_OK(ftruncate(fd, size));
    int* buf = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    ASSERT(buf != MAP_FAILED);
    for (int i = 0; i < 30000; ++i)
        ASSERT(buf[i] == i);
    for (int i = 30000; i < 50000; ++i)
        buf[i] = i;
    ASSERT_OK(close(fd));
    ASSERT_OK(munmap(buf, size));
    exit(0);
}

static void test_fs(void) {
    puts("File system");

    unlink("/tmp/test-fs/bar");
    unlink("/tmp/test-fs/baz");
    unlink("/tmp/test-fs/qux");
    rmdir("/tmp/test-fs");

    ASSERT_OK(mkdir("/tmp/test-fs", 0));

    ASSERT_ERR(open("/tmp/test-fs/bar", 0));
    ASSERT(errno == ENOENT);

    ASSERT_OK(open("/tmp/test-fs/bar", O_CREAT | O_EXCL, 0));
    ASSERT_OK(open("/tmp/test-fs/bar", O_CREAT, 0));

    ASSERT_ERR(open("/tmp/test-fs/bar", O_CREAT | O_EXCL, 0));
    ASSERT(errno == EEXIST);

    ASSERT_ERR(open("/tmp/test-fs/bar/baz", 0));
    ASSERT(errno == ENOTDIR);

    {
        int fd = open("/tmp/test-fs/qux", O_WRONLY | O_CREAT | O_EXCL);
        ASSERT_OK(fd);
        int* buf = malloc(50000 * sizeof(int));
        ASSERT(buf);
        for (int i = 0; i < 50000; ++i)
            buf[i] = i;
        ASSERT(write(fd, buf, 1000 * sizeof(int)) == 1000 * sizeof(int));
        ASSERT(write(fd, buf, 50000 * sizeof(int)) == 50000 * sizeof(int));
        free(buf);
        ASSERT_OK(close(fd));
    }
    {
        int fd = open("/tmp/test-fs/qux", O_RDWR);
        ASSERT_OK(fd);
        int* buf = malloc(50000 * sizeof(int));
        ASSERT(buf);
        ASSERT(read(fd, buf, 1000 * sizeof(int)) == 1000 * sizeof(int));
        for (int i = 0; i < 1000; ++i)
            ASSERT(buf[i] == i);
        ASSERT(read(fd, buf, 50000 * sizeof(int)) == 50000 * sizeof(int));
        for (int i = 0; i < 50000; ++i)
            ASSERT(buf[i] == i);
        ASSERT_OK(ftruncate(fd, 1000 * sizeof(int)));
        for (int i = 0; i < 1000; ++i)
            buf[i] = 5 * i;
        ASSERT(write(fd, buf, 1000 * sizeof(int)) == 1000 * sizeof(int));
        ASSERT_OK(close(fd));
        free(buf);
    }
    {
        int fd = open("/tmp/test-fs/qux", O_RDWR);
        ASSERT_OK(fd);
        int* buf = malloc(50000 * sizeof(int));
        ASSERT(buf);
        ASSERT(read(fd, buf, 1000 * sizeof(int)) == 1000 * sizeof(int));
        for (int i = 0; i < 1000; ++i)
            ASSERT(buf[i] == i);
        ASSERT(read(fd, buf, 50000 * sizeof(int)) == 50000 * sizeof(int));
        for (int i = 0; i < 50000; ++i)
            ASSERT(buf[i] == 0);
        ASSERT(read(fd, buf, 1000 * sizeof(int)) == 1000 * sizeof(int));
        for (int i = 0; i < 1000; ++i)
            ASSERT(buf[i] == 5 * i);
        ASSERT_OK(ftruncate(fd, 1000 * sizeof(int)));
        ASSERT_OK(close(fd));
    }
    {
        int fd = open("/tmp/test-fs/qux", O_RDONLY);
        ASSERT_OK(fd);
        int* buf = malloc(50000 * sizeof(int));
        ASSERT(buf);
        ASSERT(read(fd, buf, 1000 * sizeof(int)) == 1000 * sizeof(int));
        for (int i = 0; i < 1000; ++i)
            ASSERT(buf[i] == i);
        ASSERT(read(fd, buf, 50000 * sizeof(int)) == 0);
        ASSERT_OK(close(fd));
    }
    unlink("/dev/shm/test-fs");
    {
        int fd = open("/dev/shm/test-fs", O_RDWR | O_CREAT | O_EXCL);
        ASSERT_OK(fd);
        size_t size = 30000 * sizeof(int);
        ASSERT_OK(ftruncate(fd, size));
        int* buf = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        ASSERT(buf != MAP_FAILED);
        for (int i = 0; i < 30000; ++i)
            buf[i] = i;
        pid_t pid = fork();
        ASSERT_OK(pid);
        if (pid == 0)
            shm_reader();
        ASSERT_OK(waitpid(pid, NULL, 0));
        ASSERT_OK(close(fd));
        ASSERT_OK(munmap(buf, size));
    }
    {
        int fd = open("/dev/shm/test-fs", O_RDWR);
        ASSERT_OK(fd);
        size_t size = 50000 * sizeof(int);
        int* buf = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
        ASSERT(buf != MAP_FAILED);
        for (int i = 0; i < 30000; ++i)
            ASSERT(buf[i] == i);
        for (int i = 30000; i < 50000; ++i)
            ASSERT(buf[i] == i);
        ASSERT_OK(munmap(buf, size));
        ASSERT_OK(close(fd));
    }
    {
        int fd = open("/dev/shm/test-fs", O_RDWR);
        ASSERT_OK(fd);
        size_t size = 50000 * sizeof(int);
        int* buf = malloc(size);
        ASSERT(buf);
        ASSERT((size_t)read(fd, buf, size) == size);
        ASSERT_OK(close(fd));
        for (int i = 0; i < 30000; ++i)
            ASSERT(buf[i] == i);
        for (int i = 30000; i < 50000; ++i)
            ASSERT(buf[i] == i);
        free(buf);
    }
}

static size_t read_all(int fd, unsigned char* buf, size_t count) {
    size_t total = 0;
    while (total < count) {
        ssize_t n = read(fd, buf + total, count - total);
        ASSERT_OK(n);
        total += n;
    }
    return total;
}

static size_t write_all(int fd, unsigned char* buf, size_t count) {
    size_t total = 0;
    while (total < count) {
        ssize_t nwritten = write(fd, buf + total, count - total);
        ASSERT_OK(nwritten);
        total += nwritten;
    }
    return total;
}

static noreturn void socket_receiver(void) {
    int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    ASSERT_OK(sockfd);
    sockaddr_un addr = {AF_UNIX, "/tmp/test-socket"};
    ASSERT_OK(connect(sockfd, (const sockaddr*)&addr, sizeof(sockaddr_un)));
    static unsigned buf[1024];
    size_t total = 0;
    for (size_t i = 0; i < 55000; i += 1024) {
        size_t s = MIN(1024, 55000 - i) * sizeof(unsigned);
        ASSERT(read_all(sockfd, (unsigned char*)buf, s) == s);
        for (size_t j = 0; j < s / sizeof(unsigned); ++j)
            ASSERT(buf[j] == total / sizeof(unsigned) + j);
        total += s;
    }
    ASSERT_OK(close(sockfd));
    exit(0);
}

static void test_socket(void) {
    puts("Socket");

    int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    ASSERT_OK(sockfd);
    unlink("/tmp/test-socket");
    sockaddr_un addr = {AF_UNIX, "/tmp/test-socket"};
    ASSERT_OK(bind(sockfd, (const sockaddr*)&addr, sizeof(sockaddr_un)));
    ASSERT_OK(listen(sockfd, 5));

    pid_t pid1 = fork();
    ASSERT_OK(pid1);
    if (pid1 == 0)
        socket_receiver();
    pid_t pid2 = fork();
    ASSERT_OK(pid2);
    if (pid2 == 0)
        socket_receiver();

    int peer_fd1 = accept(sockfd, NULL, NULL);
    ASSERT_OK(peer_fd1);
    int peer_fd2 = accept(sockfd, NULL, NULL);
    ASSERT_OK(peer_fd2);

    static unsigned buf[10000];
    for (size_t j = 0; j < 10000; ++j)
        buf[j] = j;
    for (size_t i = 0; i < 55000; i += 10000) {
        size_t s = MIN(10000, 55000 - i) * sizeof(unsigned);
        ASSERT(write_all(peer_fd1, (unsigned char*)buf, s) == s);
        ASSERT(write_all(peer_fd2, (unsigned char*)buf, s) == s);
        for (size_t j = 0; j < 10000; ++j)
            buf[j] += 10000;
    }
    ASSERT_OK(waitpid(pid1, NULL, 0));
    ASSERT_OK(waitpid(pid2, NULL, 0));

    ASSERT_OK(close(sockfd));
    ASSERT_OK(close(peer_fd1));
    ASSERT_OK(close(peer_fd2));
}

static void* shared_mmap_addr;

static void mmap_reader(void) {
    for (size_t i = 0; i < 100; ++i)
        ASSERT(((uint32_t*)shared_mmap_addr)[i] == i);
    ASSERT_OK(munmap(shared_mmap_addr, 5000));
    exit(0);
}

static void test_mmap_shared(void) {
    puts("mmap(MAP_SHARED)");
    size_t size = 5000;
    shared_mmap_addr = mmap(NULL, size, PROT_READ | PROT_WRITE,

                            MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    ASSERT(shared_mmap_addr != MAP_FAILED);
    for (size_t i = 0; i < 100; ++i)
        ((uint32_t*)shared_mmap_addr)[i] = i;
    pid_t pid = fork();
    ASSERT_OK(pid);
    if (pid == 0)
        mmap_reader();
    ASSERT_OK(waitpid(pid, NULL, 0));
    ASSERT_OK(munmap(shared_mmap_addr, size));
}

static void test_framebuffer(void) {
    puts("Framebuffer");

    int fd = open("/dev/fb0", O_RDWR);
    if (fd < 0) {
        ASSERT(errno == ENOENT);
        return;
    }

    struct fb_info fb_info;
    ASSERT_OK(ioctl(fd, FBIOGET_INFO, &fb_info));

    size_t size = fb_info.pitch * fb_info.height;
    void* fb = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    ASSERT_OK(close(fd));
    ASSERT(fb != MAP_FAILED);
    void* buf = malloc(size);
    ASSERT(buf);
    memcpy(buf, fb, size);
    memcpy(fb, buf, size);
    free(buf);
    ASSERT_OK(munmap(fb, size));
}

static void test_malloc(void) {
    puts("malloc");
    free(malloc(0));
    for (size_t i = 0; i < 10000; ++i) {
        void* buf = malloc(1);
        ASSERT(buf);
        void* buf2 = malloc(100000);
        ASSERT(buf2);
        free(buf);
        free(buf2);
    }
}

int main(void) {
    test_fs();
    test_socket();
    test_mmap_shared();
    test_framebuffer();
    test_malloc();

    return EXIT_SUCCESS;
}
