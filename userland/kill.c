#include <extra.h>
#include <signal.h>
#include <signum.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <escp.h>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        dprintf(STDERR_FILENO, "%susage: %skill %s<%sprocess-id%s>%s\n", F_MAGENTA, F_GREEN, F_BLUE, F_GREEN, F_BLUE, RESET);
        return EXIT_FAILURE;
    }
    const char* pid_str = argv[1];
    if (!str_is_uint(pid_str)) {
        dprintf(STDERR_FILENO, "%sillegal pid: %s%s\n", F_MAGENTA, F_RED, pid_str, RESET);
        return EXIT_FAILURE;
    }
    pid_t pid = atoi(pid_str);
    if (kill(pid, SIGTERM) < 0) {
        perror("pid");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
