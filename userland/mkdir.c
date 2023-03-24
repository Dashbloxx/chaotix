#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <escp.h>

int main(int argc, char* const argv[]) {
    if (argc < 2) {
        dprintf(STDERR_FILENO, "%susage: %smkdir %s<%sdirectory%s>%s\n", F_MAGENTA, F_GREEN, F_BLUE, F_GREEN, F_BLUE, RESET);
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
