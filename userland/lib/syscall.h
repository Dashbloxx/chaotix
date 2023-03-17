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
