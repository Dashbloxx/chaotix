#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
    if (argc != 3) {
        dprintf(STDERR_FILENO, "Usage: mv SOURCE DEST\n");
        return EXIT_FAILURE;
    }
    if (rename(argv[1], argv[2]) < 0) {
        perror("rename");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
