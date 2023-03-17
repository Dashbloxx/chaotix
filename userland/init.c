#include <fcntl.h>
#include <panic.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

static pid_t spawn(char* filename) {
    pid_t pid = fork();
    if (pid < 0)
        return -1;
    if (pid == 0) {
        if (setpgid(0, 0) < 0) {
            perror("setpgid");
            abort();
        }

        char* argv[] = {filename, NULL};
        static char* envp[] = {"PATH=/bin", "HOME=/root", NULL};
        if (execve(filename, argv, envp) < 0) {
            perror("execve");
            abort();
        }
    }
    return pid;
}

int main(void) {
    ASSERT(getpid() == 1);
    ASSERT(open("/dev/console", O_RDONLY) == STDIN_FILENO);
    ASSERT(open("/dev/console", O_WRONLY) == STDOUT_FILENO);
    ASSERT(open("/dev/console", O_WRONLY) == STDERR_FILENO);

    chdir("/root");

    spawn("/bin/mouse-cursor");

    for (;;) {
        pid_t pid = spawn("/bin/sh");
        if (pid < 0) {
            perror("spawn");
            continue;
        }
        while (waitpid(-1, NULL, 0) != pid)
            ;
    }

    UNREACHABLE();
}
