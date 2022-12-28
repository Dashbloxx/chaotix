#include <extra.h>
#include <signal.h>
#include <signum.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        dprintf(STDERR_FILENO, "Usage: kill PID\n");
        return EXIT_FAILURE;
    }
    const char* pid_str = argv[1];
    if (!str_is_uint(pid_str)) {
        dprintf(STDERR_FILENO, "Illegal pid: %s\n", pid_str);
        return EXIT_FAILURE;
    }
    pid_t pid = atoi(pid_str);
    if (kill(pid, SIGTERM) < 0) {
        perror("pid");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
