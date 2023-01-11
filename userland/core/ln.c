#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
    if (argc != 3) {
        dprintf(STDERR_FILENO, "\x1b[37;1mln: \x1b[32;1musage: \x1b[0mln HOST LINK\n");
        return EXIT_FAILURE;
    }
    if (link(argv[1], argv[2]) < 0) {
        perror("link");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
