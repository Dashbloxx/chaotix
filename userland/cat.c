#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUF_SIZE 1024

static int dump_file(const char* filename) {
    int fd = strcmp(filename, "-") ? open(filename, O_RDONLY) : 0;
    if (fd < 0)
        return -1;
    for (;;) {
        static char buf[BUF_SIZE];
        ssize_t nread = read(fd, buf, BUF_SIZE);
        if (nread < 0) {
            if (fd > 0)
                close(fd);
            return -1;
        }
        if (nread == 0)
            break;
        if (write(STDOUT_FILENO, buf, nread) < 0)
            return -1;
    }
    if (fd > 0)
        close(fd);
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        if (dump_file("-") < 0) {
            perror("dump_file");
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }
    for (int i = 1; i < argc; ++i) {
        if (dump_file(argv[i]) < 0) {
            perror("dump_file");
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}
