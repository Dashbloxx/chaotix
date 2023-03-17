/*
    The xv6 software is:

    Copyright (c) 2006-2018 Frans Kaashoek, Robert Morris, Russ Cox,
                            Massachusetts Institute of Technology

    Permission is hereby granted, free of charge, to any person obtaining
    a copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to use, copy, modify, merge, publish,
    distribute, sublicense, and/or sell copies of the Software, and to
    permit persons to whom the Software is furnished to do so, subject to
    the following conditions:

    The above copyright notice and this permission notice shall be
    included in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
    NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
    LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
    OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
    WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <dirent.h>
#include <fcntl.h>
#include <panic.h>
#include <signal.h>
#include <signum.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAXFILE 1024
#define MAXARG 1024

static pid_t wait(void) { return waitpid(-1, NULL, 0); }

static int exec(const char* filename, char* const argv[]) {
    static char* const envp[] = {NULL};
    return execve(filename, argv, envp);
}

char buf[8192];
char name[3];
char* echoargv[] = {"echo", "ALL", "TESTS", "PASSED", 0};

// does chdir() call iput(p->cwd) in a transaction?
void iputtest(void) {
    printf("iput test\n");

    ASSERT_OK(mkdir("iputdir", 0));
    ASSERT_OK(chdir("iputdir"));
    ASSERT_OK(rmdir("../iputdir"));
    ASSERT_OK(chdir("/"));
    printf("iput test ok\n");
}

// does exit() call iput(p->cwd) in a transaction?
void exitiputtest(void) {
    int pid;

    printf("exitiput test\n");

    pid = fork();
    ASSERT_OK(pid);
    if (pid == 0) {
        ASSERT_OK(mkdir("iputdir", 0));
        ASSERT_OK(chdir("iputdir"));
        ASSERT_OK(rmdir("../iputdir"));
        exit(0);
    }
    wait();
    printf("exitiput test ok\n");
}

// does the error path in open() for attempt to write a
// directory call iput() in a transaction?
// needs a hacked kernel that pauses just after the namei()
// call in sys_open():
//    if((ip = namei(path)) == 0)
//      return -1;
//    {
//      int i;
//      for(i = 0; i < 10000; i++)
//        yield();
//    }
void openiputtest(void) {
    int pid;

    printf("openiput test\n");
    ASSERT_OK(mkdir("oidir", 0));
    pid = fork();
    ASSERT_OK(pid);
    if (pid == 0) {
        ASSERT_ERR(open("oidir", O_RDWR));
        exit(0);
    }
    sleep(1);
    ASSERT_OK(rmdir("oidir"));
    wait();
    printf("openiput test ok\n");
}

// simple file system tests

void opentest(void) {
    int fd;

    printf("open test\n");
    fd = open("/bin/echo", O_RDONLY);
    ASSERT_OK(fd);
    close(fd);
    fd = open("doesnotexist", O_RDONLY);
    ASSERT_ERR(fd);
    printf("open test ok\n");
}

void writetest(void) {
    int fd;
    int i;

    printf("small file test\n");
    fd = open("small", O_CREAT | O_RDWR);
    ASSERT_OK(fd);
    for (i = 0; i < 100; i++) {
        ASSERT(write(fd, "aaaaaaaaaa", 10) == 10);
        ASSERT(write(fd, "bbbbbbbbbb", 10) == 10);
    }
    printf("writes ok\n");
    close(fd);
    fd = open("small", O_RDONLY);
    ASSERT_OK(fd);
    i = read(fd, buf, 2000);
    ASSERT(i == 2000);
    close(fd);

    ASSERT_OK(unlink("small"));
    printf("small file test ok\n");
}

void writetest1(void) {
    int i;
    int fd;
    int n;

    printf("big files test\n");

    fd = open("big", O_CREAT | O_RDWR);
    ASSERT_OK(fd);

    for (i = 0; i < MAXFILE; i++) {
        ((int*)buf)[0] = i;
        ASSERT(write(fd, buf, 512) == 512);
    }

    close(fd);

    fd = open("big", O_RDONLY);
    ASSERT_OK(fd);

    n = 0;
    for (;;) {
        i = read(fd, buf, 512);
        if (i == 0) {
            ASSERT(n != MAXFILE - 1);
            break;
        }
        ASSERT(i == 512);
        ASSERT(((int*)buf)[0] == n);
        n++;
    }
    close(fd);
    ASSERT_OK(unlink("big"));
    printf("big files ok\n");
}

void createtest(void) {
    int i;
    int fd;

    printf("many creates, followed by unlink test\n");

    name[0] = 'a';
    name[2] = '\0';
    for (i = 0; i < 52; i++) {
        name[1] = '0' + i;
        fd = open(name, O_CREAT | O_RDWR);
        close(fd);
    }
    name[0] = 'a';
    name[2] = '\0';
    for (i = 0; i < 52; i++) {
        name[1] = '0' + i;
        unlink(name);
    }
    printf("many creates, followed by unlink; ok\n");
}

void dirtest(void) {
    printf("mkdir test\n");

    ASSERT_OK(mkdir("dir0", 0));

    ASSERT_OK(chdir("dir0"));

    ASSERT_OK(chdir(".."));

    ASSERT_OK(unlink("dir0"));
    printf("mkdir test ok\n");
}

void exectest(void) {
    printf("exec test\n");
    ASSERT_OK(exec("/bin/echo", echoargv));
}

static ssize_t write_all(int fd, void* buf, size_t count) {
    unsigned char* chars = (unsigned char*)buf;
    size_t total = 0;
    while (total < count) {
        ssize_t nwritten = write(fd, chars + total, count - total);
        if (nwritten < 0)
            return -1;
        total += nwritten;
    }
    return total;
}

// simple fork and pipe read/write

void pipe1(void) {
    int fds[2];
    int pid;
    int seq;
    int i;
    int n;
    size_t cc;
    int total;

    ASSERT_OK(pipe(fds));
    pid = fork();
    ASSERT_OK(pid);
    seq = 0;
    if (pid == 0) {
        close(fds[0]);
        for (n = 0; n < 5; n++) {
            for (i = 0; i < 1033; i++)
                buf[i] = seq++;
            ASSERT(write_all(fds[1], buf, 1033) == 1033);
        }
        exit(0);
    } else {
        close(fds[1]);
        total = 0;
        cc = 1;
        while ((n = read(fds[0], buf, cc)) > 0) {
            for (i = 0; i < n; i++)
                ASSERT((buf[i] & 0xff) == (seq++ & 0xff));
            total += n;
            cc = cc * 2;
            if (cc > sizeof(buf))
                cc = sizeof(buf);
        }
        ASSERT(total == 5 * 1033);
        close(fds[0]);
        wait();
    }
    printf("pipe1 ok\n");
}

// meant to be run w/ at most two CPUs
void preempt(void) {
    int pid1;
    int pid2;
    int pid3;
    int pfds[2];

    printf("preempt: ");
    pid1 = fork();
    if (pid1 == 0)
        for (;;)
            ;

    pid2 = fork();
    if (pid2 == 0)
        for (;;)
            ;

    pipe(pfds);
    pid3 = fork();
    if (pid3 == 0) {
        close(pfds[0]);
        ASSERT(write(pfds[1], "x", 1) == 1);
        close(pfds[1]);
        for (;;)
            ;
    }

    close(pfds[1]);
    ASSERT(read(pfds[0], buf, sizeof(buf)) == 1);
    close(pfds[0]);
    printf("kill... ");
    kill(pid1, SIGKILL);
    kill(pid2, SIGKILL);
    kill(pid3, SIGKILL);
    printf("wait... ");
    wait();
    wait();
    wait();
    printf("preempt ok\n");
}

// try to find any races between exit and wait
void exitwait(void) {
    int i;
    int pid;

    for (i = 0; i < 100; i++) {
        pid = fork();
        ASSERT_OK(pid);
        if (pid)
            ASSERT(wait() == pid);
        else
            exit(0);
    }
    printf("exitwait ok\n");
}

void mem(void) {
    void* m1;
    void* m2;
    int pid;
    int ppid;

    printf("mem test\n");
    ppid = getpid();
    pid = fork();
    if (pid == 0) {
        m1 = 0;
        while ((m2 = malloc(10001)) != 0) {
            *(char**)m2 = m1;
            m1 = m2;
        }
        while (m1) {
            m2 = *(char**)m1;
            free(m1);
            m1 = m2;
        }
        m1 = malloc(1024 * 20);
        if (m1 == 0) {
            kill(ppid, SIGKILL);
            PANIC("couldn't allocate mem?!!");
        }
        free(m1);
        printf("mem ok\n");
        exit(0);
    } else {
        wait();
    }
}

// More file system tests

// two processes write to the same file descriptor
// is the offset shared? does inode locking work?
void sharedfd(void) {
    int fd;
    int pid;
    size_t i;
    int n;
    int nc;
    int np;
    char buf[10];

    printf("sharedfd test\n");

    unlink("sharedfd");
    fd = open("sharedfd", O_CREAT | O_RDWR);
    ASSERT_OK(fd);
    pid = fork();
    memset(buf, pid == 0 ? 'c' : 'p', sizeof(buf));
    for (i = 0; i < 1000; i++)
        ASSERT(write(fd, buf, sizeof(buf)) == sizeof(buf));
    if (pid == 0)
        exit(0);
    else
        wait();
    close(fd);
    fd = open("sharedfd", O_RDONLY);
    ASSERT_OK(fd);
    nc = np = 0;
    while ((n = read(fd, buf, sizeof(buf))) > 0) {
        for (i = 0; i < sizeof(buf); i++) {
            if (buf[i] == 'c')
                nc++;
            if (buf[i] == 'p')
                np++;
        }
    }
    close(fd);
    unlink("sharedfd");
    ASSERT_OK(nc == 10000 && np == 10000);
}

// four processes write different files at the same
// time, to test block allocation.
void fourfiles(void) {
    int fd;
    int pid;
    int i;
    int j;
    int n;
    int total;
    int pi;
    char* names[] = {"f0", "f1", "f2", "f3"};
    char* fname;

    printf("fourfiles test\n");

    for (pi = 0; pi < 4; pi++) {
        fname = names[pi];
        unlink(fname);

        pid = fork();
        ASSERT_OK(pid);

        if (pid == 0) {
            fd = open(fname, O_CREAT | O_RDWR);
            ASSERT_OK(fd);

            memset(buf, '0' + pi, 512);
            for (i = 0; i < 12; i++)
                ASSERT((n = write(fd, buf, 500)) == 500);
            exit(0);
        }
    }

    for (pi = 0; pi < 4; pi++)
        wait();

    for (i = 0; i < 2; i++) {
        fname = names[i];
        fd = open(fname, O_RDONLY);
        total = 0;
        while ((n = read(fd, buf, sizeof(buf))) > 0) {
            for (j = 0; j < n; j++)
                ASSERT(buf[j] == '0' + i);
            total += n;
        }
        close(fd);
        ASSERT(total == 12 * 500);
        unlink(fname);
    }

    printf("fourfiles ok\n");
}

// four processes create and delete different files in same directory
void createdelete(void) {
    enum { N = 20 };
    int pid;
    int i;
    int fd;
    int pi;
    char name[32];

    printf("createdelete test\n");

    for (pi = 0; pi < 4; pi++) {
        pid = fork();
        ASSERT_OK(pid);

        if (pid == 0) {
            name[0] = 'p' + pi;
            name[2] = '\0';
            for (i = 0; i < N; i++) {
                name[1] = '0' + i;
                fd = open(name, O_CREAT | O_RDWR);
                ASSERT_OK(fd);
                close(fd);
                if (i > 0 && (i % 2) == 0) {
                    name[1] = '0' + (i / 2);
                    ASSERT_OK(unlink(name));
                }
            }
            exit(0);
        }
    }

    for (pi = 0; pi < 4; pi++)
        wait();

    name[0] = name[1] = name[2] = 0;
    for (i = 0; i < N; i++) {
        for (pi = 0; pi < 4; pi++) {
            name[0] = 'p' + pi;
            name[1] = '0' + i;
            fd = open(name, O_RDONLY);
            if ((i == 0 || i >= N / 2) && fd < 0)
                PANIC("oops createdelete didn't exist");
            else if ((i >= 1 && i < N / 2) && fd >= 0)
                PANIC("oops createdelete did exist");
            if (fd >= 0)
                close(fd);
        }
    }

    for (i = 0; i < N; i++) {
        for (pi = 0; pi < 4; pi++) {
            name[0] = 'p' + i;
            name[1] = '0' + i;
            unlink(name);
        }
    }

    printf("createdelete ok\n");
}

// can I unlink a file and still read it?
void unlinkread(void) {
    int fd;
    int fd1;

    printf("unlinkread test\n");
    fd = open("unlinkread", O_CREAT | O_RDWR);
    ASSERT_OK(fd);
    write(fd, "hello", 5);
    close(fd);

    fd = open("unlinkread", O_RDWR);
    ASSERT_OK(fd);
    ASSERT_OK(unlink("unlinkread"));

    fd1 = open("unlinkread", O_CREAT | O_RDWR);
    write(fd1, "yyy", 3);
    close(fd1);

    ASSERT(read(fd, buf, sizeof(buf)) == 5);
    ASSERT(buf[0] == 'h');
    ASSERT(write(fd, buf, 10) == 10);
    close(fd);
    unlink("unlinkread");
    printf("unlinkread ok\n");
}

void linktest(void) {
    int fd;

    printf("linktest\n");

    unlink("lf1");
    unlink("lf2");

    fd = open("lf1", O_CREAT | O_RDWR);
    ASSERT_OK(fd);
    ASSERT(write(fd, "hello", 5) == 5);
    close(fd);

    ASSERT_OK(link("lf1", "lf2"));
    unlink("lf1");

    ASSERT_ERR(open("lf1", O_RDONLY));

    fd = open("lf2", O_RDONLY);
    ASSERT_OK(fd);
    ASSERT(read(fd, buf, sizeof(buf)) == 5);
    close(fd);

    ASSERT_ERR(link("lf2", "lf2"));

    unlink("lf2");
    ASSERT_ERR(link("lf2", "lf1"));

    ASSERT_ERR(link(".", "lf1"));

    printf("linktest ok\n");
}

// test concurrent create/link/unlink of the same file
void concreate(void) {
    char file[3];
    int i;
    int pid;
    int n;
    int fd;
    char fa[40];

    printf("concreate test\n");
    file[0] = 'C';
    file[2] = '\0';
    for (i = 0; i < 40; i++) {
        file[1] = '0' + i;
        unlink(file);
        pid = fork();
        if ((pid && (i % 3) == 1) || (pid == 0 && (i % 5) == 1)) {
            link("C0", file);
        } else {
            fd = open(file, O_CREAT | O_RDWR);
            ASSERT_OK(fd);
            close(fd);
        }
        if (pid == 0)
            exit(0);
        else
            wait();
    }

    memset(fa, 0, sizeof(fa));
    DIR* dirp = opendir(".");
    n = 0;
    for (;;) {
        struct dirent* de = readdir(dirp);
        if (!de)
            break;
        if (de->d_name[0] == 'C' && de->d_name[2] == '\0') {
            i = de->d_name[1] - '0';
            ASSERT(i >= 0 && i < (int)sizeof(fa));
            ASSERT(!fa[i]);
            fa[i] = 1;
            n++;
        }
    }
    closedir(dirp);

    ASSERT(n == 40);

    for (i = 0; i < 40; i++) {
        file[1] = '0' + i;
        pid = fork();
        ASSERT_OK(pid);
        if (((i % 3) == 0 && pid == 0) || ((i % 3) == 1 && pid != 0)) {
            close(open(file, O_RDONLY));
            close(open(file, O_RDONLY));
            close(open(file, O_RDONLY));
            close(open(file, O_RDONLY));
        } else {
            unlink(file);
            unlink(file);
            unlink(file);
            unlink(file);
        }
        if (pid == 0)
            exit(0);
        else
            wait();
    }

    printf("concreate ok\n");
}

// another concurrent link/unlink/create test,
// to look for deadlocks.
void linkunlink(void) {
    int pid;
    int i;

    printf("linkunlink test\n");

    pid = fork();
    ASSERT_OK(pid);

    unsigned int x = (pid ? 1 : 97);
    for (i = 0; i < 100; i++) {
        x = x * 1103515245 + 12345;
        if ((x % 3) == 0) {
            close(open("x", O_RDWR | O_CREAT));
        } else if ((x % 3) == 1) {
            link("/bin/cat", "x");
        } else {
            unlink("x");
        }
    }

    if (pid)
        wait();
    else
        exit(0);

    printf("linkunlink ok\n");
}

// directory that uses indirect blocks
void bigdir(void) {
    int i;
    int fd;
    char name[10];

    printf("bigdir test\n");
    unlink("bd");

    fd = open("bd", O_CREAT);
    ASSERT_OK(fd);
    close(fd);

    for (i = 0; i < 500; i++) {
        name[0] = 'x';
        name[1] = '0' + (i / 64);
        name[2] = '0' + (i % 64);
        name[3] = '\0';
        ASSERT_OK(link("bd", name));
    }

    unlink("bd");
    for (i = 0; i < 500; i++) {
        name[0] = 'x';
        name[1] = '0' + (i / 64);
        name[2] = '0' + (i % 64);
        name[3] = '\0';
        ASSERT_OK(unlink(name));
    }

    printf("bigdir ok\n");
}

void subdir(void) {
    int fd;
    int cc;

    printf("subdir test\n");

    unlink("ff");
    ASSERT_OK(mkdir("dd", 0));

    fd = open("dd/ff", O_CREAT | O_RDWR);
    ASSERT_OK(fd);
    write(fd, "ff", 2);
    close(fd);

    ASSERT_ERR(unlink("dd"));

    ASSERT_OK(mkdir("/dd/dd", 0));

    fd = open("dd/dd/ff", O_CREAT | O_RDWR);
    ASSERT_OK(fd);
    write(fd, "FF", 2);
    close(fd);

    fd = open("dd/dd/../ff", O_RDONLY);
    ASSERT_OK(fd);
    cc = read(fd, buf, sizeof(buf));
    ASSERT(cc == 2 && buf[0] == 'f');
    close(fd);

    ASSERT_OK(link("dd/dd/ff", "dd/dd/ffff"));

    ASSERT_OK(unlink("dd/dd/ff"));
    ASSERT_ERR(open("dd/dd/ff", O_RDONLY));

    ASSERT_OK(chdir("dd"));
    ASSERT_OK(chdir("dd/../../dd"));
    ASSERT_OK(chdir("dd/../../../dd"));
    ASSERT_OK(chdir("./.."));

    fd = open("dd/dd/ffff", O_RDONLY);
    ASSERT_OK(fd);
    ASSERT(read(fd, buf, sizeof(buf)) == 2);
    close(fd);

    ASSERT_ERR(open("dd/dd/ff", O_RDONLY));

    ASSERT_ERR(open("dd/ff/ff", O_CREAT | O_RDWR));
    ASSERT_ERR(open("dd/xx/ff", O_CREAT | O_RDWR));
    ASSERT_ERR(open("dd", O_CREAT | O_EXCL));
    ASSERT_ERR(open("dd", O_RDWR));
    ASSERT_ERR(open("dd", O_WRONLY));
    ASSERT_ERR(link("dd/ff/ff", "dd/dd/xx"));
    ASSERT_ERR(link("dd/xx/ff", "dd/dd/xx"));
    ASSERT_ERR(link("dd/ff", "dd/dd/ffff"));
    ASSERT_ERR(mkdir("dd/ff/ff", 0));
    ASSERT_ERR(mkdir("dd/xx/ff", 0));
    ASSERT_ERR(mkdir("dd/dd/ffff", 0));
    ASSERT_ERR(unlink("dd/xx/ff"));
    ASSERT_ERR(unlink("dd/ff/ff"));
    ASSERT_ERR(chdir("dd/ff"));
    ASSERT_ERR(chdir("dd/xx"));

    ASSERT_OK(unlink("dd/dd/ffff"));
    ASSERT_OK(unlink("dd/ff"));
    ASSERT_ERR(rmdir("dd"));
    ASSERT_OK(rmdir("dd/dd"));
    ASSERT_OK(rmdir("dd"));

    printf("subdir ok\n");
}

// test writes that are larger than the log.
void bigwrite(void) {
    int fd;
    int sz;

    printf("bigwrite test\n");

    unlink("bigwrite");
    for (sz = 499; sz < 12 * 512; sz += 471) {
        fd = open("bigwrite", O_CREAT | O_RDWR);
        ASSERT_OK(fd);
        int i;
        for (i = 0; i < 2; i++) {
            int cc = write(fd, buf, sz);
            ASSERT(cc == sz);
        }
        close(fd);
        unlink("bigwrite");
    }

    printf("bigwrite ok\n");
}

void bigfile(void) {
    int fd;
    int i;
    int total;
    int cc;

    printf("bigfile test\n");

    unlink("bigfile");
    fd = open("bigfile", O_CREAT | O_RDWR);
    ASSERT_OK(fd);
    for (i = 0; i < 20; i++) {
        memset(buf, i, 600);
        ASSERT(write(fd, buf, 600) == 600);
    }
    close(fd);

    fd = open("bigfile", O_RDONLY);
    ASSERT_OK(fd);
    total = 0;
    for (i = 0;; i++) {
        cc = read(fd, buf, 300);
        ASSERT_OK(cc);
        if (cc == 0)
            break;
        ASSERT(cc == 300);
        ASSERT(buf[0] == i / 2 && buf[299] == i / 2);
        total += cc;
    }
    close(fd);
    ASSERT(total == 20 * 600);
    unlink("bigfile");

    printf("bigfile test ok\n");
}

void fourteen(void) {
    int fd;

    // DIRSIZ is 14.
    printf("fourteen test\n");

    ASSERT_OK(mkdir("12345678901234", 0));
    ASSERT_OK(mkdir("12345678901234/123456789012345", 0));
    fd = open("12345678901234/123456789012345/123456789012345", O_CREAT);
    ASSERT_OK(fd);
    close(fd);
    fd = open("12345678901234/123456789012345/123456789012345", O_RDONLY);
    ASSERT_OK(fd);
    close(fd);

    ASSERT_ERR(mkdir("12345678901234/123456789012345", 0));
    ASSERT_ERR(mkdir("123456789012345/12345678901234", 0));

    printf("fourteen ok\n");
}

void rmdot(void) {
    printf("rmdot test\n");
    ASSERT_OK(mkdir("dots", 0));
    ASSERT_OK(chdir("dots"));
    // TODO: ASSERT_ERR(rmdir("."));
    // TODO: ASSERT_ERR(rmdir(".."));
    ASSERT_OK(chdir("/"));
    // TODO: ASSERT_ERR(rmdir("dots/."));
    // TODO: ASSERT_ERR(rmdir("dots/.."));
    ASSERT_OK(rmdir("dots"));
    printf("rmdot ok\n");
}

void dirfile(void) {
    int fd;

    printf("dir vs file\n");

    fd = open("dirfile", O_CREAT);
    ASSERT_OK(fd);
    close(fd);
    ASSERT_ERR(chdir("dirfile"));
    fd = open("dirfile/xx", O_RDONLY);
    ASSERT_ERR(fd);
    fd = open("dirfile/xx", O_CREAT);
    ASSERT_ERR(fd);
    ASSERT_ERR(mkdir("dirfile/xx", 0));
    ASSERT_ERR(unlink("dirfile/xx"));
    ASSERT_ERR(link("README", "dirfile/xx"));
    ASSERT_OK(unlink("dirfile"));

    fd = open(".", O_RDWR);
    ASSERT_ERR(fd);
    fd = open(".", O_RDONLY);
    ASSERT_ERR(write(fd, "x", 1));
    close(fd);

    printf("dir vs file OK\n");
}

// test that iput() is called at the end of _namei()
void iref(void) {
    int i;
    int fd;

    printf("empty file name\n");

    // the 50 is NINODE
    for (i = 0; i < 50 + 1; i++) {
        ASSERT_OK(mkdir("irefd", 0));
        ASSERT_OK(chdir("irefd"));

        mkdir("", 0);
        link("README", "");
        fd = open("", O_CREAT);
        if (fd >= 0)
            close(fd);
        fd = open("xx", O_CREAT);
        if (fd >= 0)
            close(fd);
        unlink("xx");
    }

    chdir("/");
    printf("empty file name OK\n");
}

// test that fork fails gracefully
// the forktest binary also does this, but it runs out of proc entries first.
// inside the bigger usertests binary, we run out of memory first.
void forktest(void) {
    int n;
    int pid;

    printf("fork test\n");

    for (n = 0; n < 1000; n++) {
        pid = fork();
        if (pid < 0)
            break;
        if (pid == 0)
            exit(0);
    }

    for (; n > 0; n--)
        ASSERT_OK(wait());

    ASSERT_ERR(wait());

    printf("fork test OK\n");
}

// does unintialized data start out zero?
char uninit[10000];
void bsstest(void) {
    size_t i;

    printf("bss test\n");
    for (i = 0; i < sizeof(uninit); i++)
        ASSERT(uninit[i] == '\0');
    printf("bss test ok\n");
}

// does exec return an error if the arguments
// are larger than a page? or does it write
// below the stack and wreck the instructions/data?
void bigargtest(void) {
    int pid;
    int fd;

    unlink("bigarg-ok");
    pid = fork();
    ASSERT_OK(pid);
    if (pid == 0) {
        static char* args[MAXARG];
        int i;
        for (i = 0; i < MAXARG - 1; i++)
            args[i] = "bigargs test: failed\n                                  "
                      "                                                        "
                      "                                                        "
                      "                                                     ";
        args[MAXARG - 1] = 0;
        printf("bigarg test\n");
        exec("/bin/echo", args);
        printf("bigarg test ok\n");
        fd = open("bigarg-ok", O_CREAT);
        close(fd);
        exit(0);
    }
    wait();
    fd = open("bigarg-ok", O_RDONLY);
    ASSERT_OK(fd);
    close(fd);
    unlink("bigarg-ok");
}

// what happens when the file system runs out of blocks?
// answer: balloc panics, so this test is not useful.
void fsfull(void) {
    int nfiles;

    printf("fsfull test\n");

    for (nfiles = 0;; nfiles++) {
        char name[64];
        name[0] = 'f';
        name[1] = '0' + nfiles / 1000;
        name[2] = '0' + (nfiles % 1000) / 100;
        name[3] = '0' + (nfiles % 100) / 10;
        name[4] = '0' + (nfiles % 10);
        name[5] = '\0';
        printf("writing %s\n", name);
        int fd = open(name, O_CREAT | O_RDWR);
        ASSERT_OK(fd);
        int total = 0;
        while (1) {
            int cc = write(fd, buf, 512);
            if (cc < 512)
                break;
            total += cc;
        }
        printf("wrote %d bytes\n", total);
        close(fd);
        if (total == 0)
            break;
    }

    while (nfiles >= 0) {
        char name[64];
        name[0] = 'f';
        name[1] = '0' + nfiles / 1000;
        name[2] = '0' + (nfiles % 1000) / 100;
        name[3] = '0' + (nfiles % 100) / 10;
        name[4] = '0' + (nfiles % 10);
        name[5] = '\0';
        unlink(name);
        nfiles--;
    }

    printf("fsfull test finished\n");
}

int main(void) {
    ASSERT_OK(chdir("/"));

    printf("usertests starting\n");

    if (open("usertests.ran", O_RDONLY) >= 0) {
        printf("already ran user tests -- please reboot\n");
        exit(0);
    }
    close(open("usertests.ran", O_CREAT));

    createdelete();
    linkunlink();
    concreate();
    fourfiles();
    sharedfd();

    bigargtest();
    bigwrite();
    bigargtest();
    bsstest();

    opentest();
    writetest();
    writetest1();
    createtest();

    openiputtest();
    exitiputtest();
    iputtest();

    mem();
    pipe1();
    preempt();
    exitwait();

    rmdot();
    fourteen();
    bigfile();
    subdir();
    linktest();
    unlinkread();
    dirfile();
    iref();
    forktest();
    bigdir(); // slow

    exectest();

    exit(0);
}
