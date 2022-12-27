#pragma once

#include <kernel/api/dirent.h>
#include <stddef.h>

typedef struct DIR DIR;

DIR* opendir(const char* name);
int closedir(DIR* dirp);
struct dirent* readdir(DIR* dirp);

long getdents(int fd, void* dirp, size_t count);
