#pragma once

#include <kernel/api/sys/mman.h>
#include <kernel/api/sys/types.h>
#include <stddef.h>

void* mmap(void* addr, size_t length, int prot, int flags, int fd,
           off_t offset);
int munmap(void* addr, size_t length);
