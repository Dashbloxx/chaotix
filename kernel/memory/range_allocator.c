/*
 *  .OOOOOO.   OOOO                                .    O8O              
 *  D8P'  `Y8B  `888                              .O8    `"'              
 * 888           888 .OO.    .OOOO.    .OOOOO.  .O888OO OOOO  OOOO    OOO 
 * 888           888P"Y88B  `P  )88B  D88' `88B   888   `888   `88B..8P'  
 * 888           888   888   .OP"888  888   888   888    888     Y888'    
 * `88B    OOO   888   888  D8(  888  888   888   888 .  888   .O8"'88B   
 *  `Y8BOOD8P'  O888O O888O `Y888""8O `Y8BOD8P'   "888" O888O O88'   888O 
 * 
 *  Chaotix is a UNIX-like operating system that consists of a kernel written in C and
 *  i?86 assembly, and userland binaries written in C.
 *     
 *  Copyright (c) 2023 Nexuss
 *  Copyright (c) 2022 mosm
 *  Copyright (c) 2006-2018 Frans Kaashoek, Robert Morris, Russ Cox, Massachusetts Institute of Technology
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

#include "memory.h"
#include <common/extra.h>
#include <kernel/boot_defs.h>
#include <kernel/kprintf.h>
#include <kernel/panic.h>

struct range {
    size_t size;
    struct range* next;
};

int range_allocator_init(range_allocator* allocator, uintptr_t start, uintptr_t end) {
    ASSERT(start % PAGE_SIZE == 0);
    ASSERT(end % PAGE_SIZE == 0);

    *allocator = (range_allocator){0};
    allocator->start = start;
    allocator->end = end;

    int rc = paging_map_to_free_pages(start, sizeof(struct range), PAGE_WRITE);
    if (IS_ERR(rc))
        return rc;

    struct range* range = (struct range*)start;
    allocator->ranges = range;
    range->size = end - start;
    range->next = NULL;
    return 0;
}

uintptr_t range_allocator_alloc(range_allocator* allocator, size_t size) {
    ASSERT(allocator->ranges);
    size = round_up(size, PAGE_SIZE);

    mutex_lock(&allocator->lock);

    struct range* prev = NULL;
    struct range* it = allocator->ranges;
    while (it && it->size < size) {
        prev = it;
        it = it->next;
    }
    if (!it) {
        kputs("Out of userland virtual address space\n");
        return -ENOMEM;
    }

    if (it->size == size) {
        if (prev) {
            prev->next = it->next;
        } else {
            ASSERT(allocator->ranges == it);
            allocator->ranges = it->next;
        }
        paging_unmap((uintptr_t)it, sizeof(struct range));
        mutex_unlock(&allocator->lock);
        return (uintptr_t)it;
    }

    uintptr_t addr = (uintptr_t)it + size;
    int rc = paging_copy_mapping(addr, (uintptr_t)it, sizeof(struct range),
                                 PAGE_WRITE);
    if (IS_ERR(rc)) {
        mutex_unlock(&allocator->lock);
        return rc;
    }
    struct range* range = (struct range*)addr;
    range->size -= size;
    paging_unmap((uintptr_t)it, sizeof(struct range));
    if (prev) {
        prev->next = range;
    } else {
        ASSERT(allocator->ranges == it);
        allocator->ranges = range;
    }

    mutex_unlock(&allocator->lock);
    return (uintptr_t)it;
}

int range_allocator_free(range_allocator* allocator, uintptr_t addr, size_t size) {
    ASSERT(addr % PAGE_SIZE == 0);
    ASSERT(allocator->ranges);
    size = round_up(size, PAGE_SIZE);
    if (addr < allocator->start || allocator->end < addr + size)
        return -EINVAL;

    mutex_lock(&allocator->lock);

    struct range* prev = NULL;
    struct range* it = allocator->ranges;
    while (it && (uintptr_t)it + it->size <= addr) {
        ASSERT(((uintptr_t)it + it->size <= addr) || (addr + size <= (uintptr_t)it));
        prev = it;
        it = it->next;
    }
    if (prev && (uintptr_t)prev + prev->size == addr) {
        prev->size += size;
        if (it && (uintptr_t)prev + prev->size == (uintptr_t)it) {
            // we're filling a gap
            prev->size += it->size;
            prev->next = it->next;
            paging_unmap((uintptr_t)it, sizeof(struct range));
        }
        mutex_unlock(&allocator->lock);
        return 0;
    }
    if (it && (uintptr_t)it == addr + size) {
        int rc = paging_copy_mapping(addr, (uintptr_t)it, sizeof(struct range), PAGE_WRITE);
        if (IS_ERR(rc)) {
            mutex_unlock(&allocator->lock);
            return rc;
        }
        struct range* range = (struct range*)addr;
        range->size += size;
        paging_unmap((uintptr_t)it, sizeof(struct range));
        if (prev) {
            prev->next = range;
        } else {
            ASSERT(allocator->ranges == it);
            allocator->ranges = range;
        }
        mutex_unlock(&allocator->lock);
        return 0;
    }

    if (prev)
        ASSERT((uintptr_t)prev + prev->size < addr);
    if (it)
        ASSERT(addr + size < (uintptr_t)it);

    int rc = paging_map_to_free_pages(addr, sizeof(struct range), PAGE_WRITE);
    if (IS_ERR(rc)) {
        mutex_unlock(&allocator->lock);
        return rc;
    }

    struct range* range = (struct range*)addr;
    range->size = size;
    range->next = it;
    if (prev)
        prev->next = range;
    else
        allocator->ranges = range;

    mutex_unlock(&allocator->lock);
    return 0;
}
