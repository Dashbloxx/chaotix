#include "growable_buf.h"
#include "api/err.h"
#include "boot_defs.h"
#include "memory/memory.h"
#include "panic.h"
#include <common/string.h>
#include <common/stdio.h>

void growable_buf_destroy(growable_buf* buf) {
    if (buf->addr)
        paging_unmap(buf->addr, buf->capacity);
}

ssize_t growable_buf_pread(growable_buf* buf, void* bytes, size_t count,
                           off_t offset) {
    if ((size_t)offset >= buf->size)
        return 0;
    if (offset + count >= buf->size)
        count = buf->size - offset;

    memcpy(bytes, (void*)(buf->addr + offset), count);
    return count;
}

NODISCARD static int grow_buf(growable_buf* buf, size_t requested_size) {
    size_t new_capacity =
        round_up(MAX(buf->capacity * 2, requested_size), PAGE_SIZE);
    if (new_capacity == 0)
        new_capacity = PAGE_SIZE;

    uintptr_t new_addr =
        range_allocator_alloc(&kernel_vaddr_allocator, new_capacity);
    if (IS_ERR(new_addr))
        return new_addr;

    if (buf->addr) {
        int rc = paging_copy_mapping(new_addr, buf->addr, buf->capacity,
                                     PAGE_WRITE | PAGE_GLOBAL);
        if (IS_ERR(rc))
            return rc;
    } else {
        ASSERT(buf->capacity == 0);
    }

    int rc = paging_map_to_free_pages(new_addr + buf->capacity,
                                      new_capacity - buf->capacity,
                                      PAGE_WRITE | PAGE_GLOBAL);
    if (IS_ERR(rc))
        return rc;

    if (buf->addr)
        memcpy((void*)new_addr, (void*)buf->addr, buf->size);
    memset((void*)(new_addr + buf->size), 0, new_capacity - buf->size);

    if (buf->addr) {
        paging_unmap(buf->addr, buf->capacity);
        int rc = range_allocator_free(&kernel_vaddr_allocator, buf->addr,
                                      buf->capacity);
        if (IS_ERR(rc))
            return rc;
    }

    buf->addr = new_addr;
    buf->capacity = new_capacity;
    return 0;
}

ssize_t growable_buf_pwrite(growable_buf* buf, const void* bytes, size_t count,
                            off_t offset) {
    size_t end = offset + count;
    if (end > buf->capacity) {
        int rc = grow_buf(buf, end);
        if (IS_ERR(rc))
            return rc;
    }

    memcpy((void*)(buf->addr + offset), bytes, count);
    if (buf->size < end)
        buf->size = end;

    return count;
}

ssize_t growable_buf_append(growable_buf* buf, const void* bytes,
                            size_t count) {
    return growable_buf_pwrite(buf, bytes, count, buf->size);
}

int growable_buf_truncate(growable_buf* buf, off_t length) {
    if ((size_t)length <= buf->size) {
        memset((void*)(buf->addr + length), 0, buf->size - length);
    } else if ((size_t)length <= buf->capacity) {
        memset((void*)(buf->addr + buf->size), 0, length - buf->size);
    } else {
        // length > capacity
        int rc = grow_buf(buf, length);
        if (IS_ERR(rc))
            return rc;
    }

    buf->size = length;
    return 0;
}

uintptr_t growable_buf_mmap(growable_buf* buf, uintptr_t addr, size_t length,
                            off_t offset, uint16_t page_flags) {
    if (offset != 0 || !(page_flags & PAGE_SHARED))
        return -ENOTSUP;

    if (length > buf->size)
        return -EINVAL;

    int rc = paging_copy_mapping(addr, buf->addr, length, page_flags);
    if (IS_ERR(rc))
        return rc;

    return addr;
}

int growable_buf_printf(growable_buf* buf, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int ret = growable_buf_vsprintf(buf, format, args);
    va_end(args);
    return ret;
}

int growable_buf_vsprintf(growable_buf* buf, const char* format, va_list args) {
    for (;;) {
        size_t max_len = buf->capacity - buf->size;
        if (max_len > 0) {
            char* dest = (char*)(buf->addr + buf->size);
            int len = vsnprintf(dest, max_len, format, args);
            if ((size_t)len < max_len) {
                buf->size += len;
                return len;
            }
        }
        int rc = grow_buf(buf, 0); // specify 0 to let grow_buf decide size
        if (IS_ERR(rc))
            return rc;
    }
}
