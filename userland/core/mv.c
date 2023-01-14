#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
    if (argc != 3) {
        dprintf(STDERR_FILENO, "\x1b[37;1mmv: \x1b[32;1musage: \x1b[0mmv FILE(S) TARGET\n");
        return EXIT_FAILURE;
    }
    if (rename(argv[1], argv[2]) < 0) {
        perror("rename");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
