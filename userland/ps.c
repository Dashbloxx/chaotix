#include <dirent.h>
#include <errno.h>
#include <extra.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void) {
    DIR* dirp = opendir("/proc");
    if (!dirp) {
        perror("opendir");
        return EXIT_FAILURE;
    }

    printf("  PID CMD\n");
    for (;;) {
        errno = 0;
        struct dirent* dent = readdir(dirp);
        if (!dent) {
            if (errno == 0)
                break;
            perror("readdir");
            return EXIT_FAILURE;
        }

        if (!str_is_uint(dent->d_name))
            continue;

        pid_t pid = atoi(dent->d_name);

        char pathname[32];
        (void)snprintf(pathname, sizeof(pathname), "/proc/%d/comm", pid);
        int fd = open(pathname, O_RDONLY);
        if (fd < 0) {
            perror("open");
            return EXIT_FAILURE;
        }
        char comm[32];
        ssize_t nread = read(fd, comm, sizeof(comm));
        close(fd);

        if (nread < 0) {
            perror("read");
            return EXIT_FAILURE;
        }
        if (nread == 0)
            continue;

        if (comm[nread - 1] == '\n')
            comm[nread - 1] = 0;

        printf("%5d %s\n", pid, comm);
    }

    closedir(dirp);
    return EXIT_SUCCESS;
}
