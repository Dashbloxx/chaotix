#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <escp.h>

int main(int argc, char* argv[]) {
    if (argc != 3) {
        dprintf(STDERR_FILENO, "%susage: %smv %s<%ssource%s> <%sdestination%s>%s\n", F_MAGENTA, F_GREEN, F_BLUE, F_GREEN, F_BLUE, F_GREEN, F_BLUE, RESET);
        return EXIT_FAILURE;
    }
    if (rename(argv[1], argv[2]) < 0) {
        perror("rename");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
