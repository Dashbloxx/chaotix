#include <ctype.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUF_SIZE 1024

static int process_file(const char* name, const char* filename) {
    int fd = strcmp(filename, "-") ? open(filename, O_RDONLY) : 0;
    if (fd < 0)
        return -1;

    size_t lines = 0;
    size_t words = 0;
    size_t bytes = 0;
    bool in_word = false;
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
        for (ssize_t i = 0; i < nread; ++i) {
            ++bytes;
            if (buf[i] == '\n')
                ++lines;
            if (isspace(buf[i])) {
                in_word = false;
            } else if (!in_word) {
                ++words;
                in_word = true;
            }
        }
    }
    if (fd > 0)
        close(fd);

    printf("%7u %7u %7u %s\n", lines, words, bytes, name);
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        if (process_file("", "-") < 0) {
            perror("process_file");
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }
    for (int i = 1; i < argc; ++i) {
        if (process_file(argv[i], argv[i]) < 0) {
            perror("process_file");
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}
