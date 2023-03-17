#pragma once

#include "api/sys/types.h"
#include "lock.h"
#include <common/extra.h>
#include <stdarg.h>

typedef struct growable_buf {
    mutex lock;
    uintptr_t addr;
    atomic_size_t capacity, size;
} growable_buf;

void growable_buf_destroy(growable_buf*);

NODISCARD ssize_t growable_buf_pread(growable_buf*, void* bytes, size_t count,
                                     off_t offset);
NODISCARD ssize_t growable_buf_pwrite(growable_buf*, const void* bytes,
                                      size_t count, off_t offset);
NODISCARD ssize_t growable_buf_append(growable_buf*, const void* bytes,
                                      size_t count);

NODISCARD int growable_buf_truncate(growable_buf*, off_t length);

NODISCARD uintptr_t growable_buf_mmap(growable_buf*, uintptr_t addr,
                                      size_t length, off_t offset,
                                      uint16_t page_flags);

int growable_buf_printf(growable_buf*, const char* format, ...);
int growable_buf_vsprintf(growable_buf*, const char* format, va_list args);
