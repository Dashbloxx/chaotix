/*
 *    __  __                      
 *   |  \/  |__ _ __ _ _ __  __ _ 
 *   | |\/| / _` / _` | '  \/ _` |
 *   |_|  |_\__,_\__, |_|_|_\__,_|
 *               |___/        
 * 
 *  Magma is a UNIX-like operating system that consists of a kernel written in C and
 *  i?86 assembly, and userland binaries written in C.
 *     
 *  Copyright (c) 2023 Nexuss, John Paul Wohlscheid, rilysh, Milton612, and FueledByCocaine
 * 
 *  This file may or may not contain code from https://github.com/mosmeh/yagura, and/or
 *  https://github.com/mit-pdos/xv6-public. Both projects have the same license as this
 *  project, and the license can be seen below:
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *  
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *  
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 *  THE SOFTWARE.
 */

#include "stdlib.h"
#include "errno.h"
#include "panic.h"
#include "signal.h"
#include "signum.h"
#include "string.h"
#include "sys/mman.h"
#include "unistd.h"
#include <extra.h>
#include <stdalign.h>
#include <stddef.h>

noreturn void abort(void) {
    kill(getpid(), SIGABRT);
    UNREACHABLE();
}

#define MALLOC_MAGIC 0xab4fde8d

struct malloc_header {
    uint32_t magic;
    size_t size;
    unsigned char data[];
};

static size_t page_size;

void* aligned_alloc(size_t alignment, size_t size) {
    if (size == 0)
        return NULL;

    if (!page_size)
        page_size = sysconf(_SC_PAGESIZE);

    ASSERT(alignment <= page_size);

    size_t data_offset =
        round_up(offsetof(struct malloc_header, data), alignment);
    size_t real_size = data_offset + size;
    void* addr = mmap(NULL, real_size, PROT_READ | PROT_WRITE,
                      MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
    if (addr == MAP_FAILED) {
        errno = ENOMEM;
        return NULL;
    }

    struct malloc_header* header = (struct malloc_header*)addr;
    header->magic = MALLOC_MAGIC;
    header->size = real_size;

    void* ptr = (void*)((uintptr_t)addr + data_offset);
    memset(ptr, 0, size);
    return ptr;
}

void* malloc(size_t size) { return aligned_alloc(alignof(max_align_t), size); }

void* calloc(size_t num, size_t size) {
    size_t total_size = num * size;
    void* ptr = malloc(total_size);
    if (!ptr)
        return NULL;
    memset(ptr, 0, total_size);
    return ptr;
}

static struct malloc_header* header_from_ptr(void* ptr) {
    ASSERT(page_size);
    uintptr_t addr = round_down((uintptr_t)ptr, page_size);
    if ((uintptr_t)ptr - addr < sizeof(struct malloc_header))
        addr -= page_size;

    struct malloc_header* header = (struct malloc_header*)addr;
    ASSERT(header->magic == MALLOC_MAGIC);
    return header;
}

void* realloc(void* ptr, size_t new_size) {
    if (!ptr)
        return malloc(new_size);
    if (new_size == 0) {
        free(ptr);
        return NULL;
    }

    struct malloc_header* old_header = header_from_ptr(ptr);

    void* new_ptr = malloc(new_size);
    if (!new_ptr)
        return NULL;
    struct malloc_header* new_header = header_from_ptr(new_ptr);

    memcpy(new_header, old_header, old_header->size);
    free(ptr);

    return new_ptr;
}

void free(void* ptr) {
    if (!ptr)
        return;
    struct malloc_header* header = header_from_ptr(ptr);
    ASSERT_OK(munmap((void*)header, header->size));
}

char* getenv(const char* name) {
    for (char** env = environ; *env; ++env) {
        char* s = strchr(*env, '=');
        if (!s)
            continue;
        size_t len = s - *env;
        if (len > 0 && !strncmp(*env, name, len))
            return s + 1;
    }
    return NULL;
}

static int rand_state = 1;

int rand(void) {
    rand_state = ((rand_state * 1103515245U) + 12345U) & 0x7fffffff;
    return rand_state;
}

void srand(unsigned seed) { rand_state = seed == 0 ? 1 : seed; }
