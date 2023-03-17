#include "dirent.h"
#include "errno.h"
#include "stdlib.h"
#include "fcntl.h"
#include "unistd.h"

typedef struct DIR {
    int fd;
    unsigned char* buf;
    size_t buf_capacity, buf_size;
    size_t buf_cursor;
} DIR;

DIR* opendir(const char* name) {
    DIR* dirp = malloc(sizeof(DIR));
    if (!dirp)
        return NULL;
    *dirp = (DIR){0};
    dirp->fd = open(name, O_RDONLY);
    if (dirp->fd < 0)
        return NULL;
    dirp->buf_capacity = 1024;
    return dirp;
}

int closedir(DIR* dirp) {
    int rc = close(dirp->fd);
    free(dirp->buf);
    free(dirp);
    return rc;
}

struct dirent* readdir(DIR* dirp) {
    if (dirp->buf_cursor >= dirp->buf_size) {
        int saved_errno = errno;
        for (;;) {
            unsigned char* new_buf = realloc(dirp->buf, dirp->buf_capacity);
            if (!new_buf)
                return NULL;
            dirp->buf = new_buf;
            errno = 0;
            ssize_t nread = getdents(dirp->fd, dirp->buf, dirp->buf_capacity);
            if (nread == 0) {
                errno = saved_errno;
                return NULL;
            }
            if (nread > 0) {
                dirp->buf_size = nread;
                break;
            }
            if (errno != EINVAL)
                return NULL;
            dirp->buf_capacity *= 2;
        }
        errno = saved_errno;
        dirp->buf_cursor = 0;
    }
    struct dirent* dent = (struct dirent*)(dirp->buf + dirp->buf_cursor);
    dirp->buf_cursor += dent->d_reclen;
    return dent;
}
