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
