#include "libgen.h"
#include "string.h"

char* basename(char* path) {
    if (!path)
        return ".";
    size_t len = strlen(path);
    if (len == 0)
        return ".";

    while (len > 1 && path[len - 1] == '/')
        path[--len] = 0;

    char* last_slash = strrchr(path, '/');
    if (!last_slash)
        return path;

    if (len == 1)
        return "/";
    return last_slash + 1;
}
