#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        dprintf(STDERR_FILENO, "Usage: rmdir DIRECTORY...\n");
        return EXIT_FAILURE;
    }
    for (int i = 1; i < argc; ++i) {
        if (rmdir(argv[i]) < 0) {
            perror("rmdir");
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}
