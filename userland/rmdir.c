#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <escp.h>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        dprintf(STDERR_FILENO, "%susage: %srmdir %s<%sdirectory-to-remove%s>%s\n", F_MAGENTA, F_GREEN, F_BLUE, F_GREEN, F_BLUE, RESET);
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
