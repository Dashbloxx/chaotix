#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
    if (argc != 3) {
        dprintf(STDERR_FILENO, "Usage: ln TARGET LINK_NAME\n");
        return EXIT_FAILURE;
    }
    if (link(argv[1], argv[2]) < 0) {
        perror("link");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
