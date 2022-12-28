#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

int main(int argc, char* const argv[]) {
    if (argc < 2) {
        dprintf(STDERR_FILENO, "Usage: mkdir DIRECTORY...\n");
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
