#include <fcntl.h>
#include <panic.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/reboot.h>
#include <sys/wait.h>
#include <unistd.h>

static int spawn(char* filename) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        abort();
    }
    if (pid == 0) {
        char* argv[] = {filename, NULL};
        static char* envp[] = {NULL};
        if (execve(argv[0], argv, envp) < 0) {
            perror("execve");
            abort();
        }
    }
    return waitpid(pid, NULL, 0);
}

int main(void) {
    ASSERT(getpid() == 1);
    ASSERT(open("/dev/console", O_RDONLY) == STDIN_FILENO);
    ASSERT(open("/dev/console", O_WRONLY) == STDOUT_FILENO);
    ASSERT(open("/dev/console", O_WRONLY) == STDERR_FILENO);

    ASSERT_OK(spawn("/bin/run-tests"));
    ASSERT_OK(spawn("/bin/xv6-usertests"));

    reboot(RB_POWEROFF);
    UNREACHABLE();
}
