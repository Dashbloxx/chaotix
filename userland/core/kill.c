#include <extra.h>
#include <signal.h>
#include <signum.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        dprintf(STDERR_FILENO, "\x1b[37;1mkill: \x1b[32;1musage: \x1b[0mkill PID\n");
        return EXIT_FAILURE;
    }
    const char* pid_str = argv[1];
    if (!str_is_uint(pid_str)) {
        dprintf(STDERR_FILENO, "\x1b[37;1mkill: \x1b[32;1merror: \x1b[0mIllegal PID: %s\n", pid_str);
        return EXIT_FAILURE;
    }
    pid_t pid = atoi(pid_str);
    if (kill(pid, SIGTERM) < 0) {
        perror("pid");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
