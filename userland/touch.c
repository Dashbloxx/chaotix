#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <escp.h>

int main(int argc, char* const argv[]) {
    if (argc < 2) {
        dprintf(STDERR_FILENO, "%susage: %stouch %s<%sfile-to-create%s>%s\n", F_MAGENTA, F_GREEN, F_BLUE, F_GREEN, F_BLUE, RESET);
        return EXIT_FAILURE;
    }

    for (int i = 1; i < argc; ++i) {
        int fd = open(argv[i], O_CREAT, 0);
        if (fd < 0) {
            perror("open");
            return EXIT_FAILURE;
        }
        close(fd);
    }

    return EXIT_SUCCESS;
}
