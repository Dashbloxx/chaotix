#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <escp.h>

#define BUF_SIZE 1024

int main(int argc, char* argv[]) {
    if (argc != 3) {
        dprintf(STDERR_FILENO, "%susage: %scp %s<%ssource%s> <%sdestination%s>%s\n", F_MAGENTA, F_GREEN, F_BLUE, F_GREEN, F_BLUE, F_GREEN, F_BLUE, RESET);
        return EXIT_FAILURE;
    }

    int src_fd = open(argv[1], O_RDONLY);
    if (src_fd < 0) {
        perror("open");
        return EXIT_FAILURE;
    }
    int dest_fd = open(argv[2], O_CREAT | O_WRONLY);
    if (dest_fd < 0) {
        perror("open");
        return EXIT_FAILURE;
    }

    for (;;) {
        static char buf[BUF_SIZE];
        ssize_t nread = read(src_fd, buf, BUF_SIZE);
        if (nread < 0) {
            perror("read");
            close(src_fd);
            close(dest_fd);
            return EXIT_FAILURE;
        }
        if (nread == 0)
            break;
        if (write(dest_fd, buf, nread) < 0) {
            perror("write");
            close(src_fd);
            close(dest_fd);
            return EXIT_FAILURE;
        }
    }

    close(src_fd);
    close(dest_fd);

    return EXIT_SUCCESS;
}
