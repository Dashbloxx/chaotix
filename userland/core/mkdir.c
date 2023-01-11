#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

int main(int argc, char* const argv[]) {
    if (argc < 2) {
        dprintf(STDERR_FILENO, "\x1b[37;1mmkdir: \x1b[32;1musage: \x1b[0mmkdir DIRECTORIE(S)...\n");
        return EXIT_FAILURE;
    }
    for (int i = 1; i < argc; ++i) {
        if (mkdir(argv[i], 0) < 0) {
            perror("mkdir");
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}
