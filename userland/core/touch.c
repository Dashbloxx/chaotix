#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char* const argv[]) {
    if (argc < 2) {
        dprintf(STDERR_FILENO, "\x1b[37;1mtouch: \x1b[32;1musage: \x1b[0mtouch FILE(S)...\n");
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
