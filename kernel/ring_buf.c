#include "ring_buf.h"
#include "api/errno.h"
#include "memory/memory.h"

#define BUF_CAPACITY 1024

int ring_buf_init(ring_buf* buf) {
    *buf = (ring_buf){0};
    buf->inner_buf = kmalloc(BUF_CAPACITY);
    if (!buf->inner_buf)
        return -ENOMEM;
    buf->write_idx = buf->read_idx = 0;
    return 0;
}

void ring_buf_destroy(ring_buf* buf) { kfree(buf->inner_buf); }

bool ring_buf_is_empty(const ring_buf* buf) {
    return buf->write_idx == buf->read_idx;
}

bool ring_buf_is_full(const ring_buf* buf) {
    return (buf->write_idx + 1) % BUF_CAPACITY == buf->read_idx;
}

ssize_t ring_buf_read(ring_buf* buf, void* bytes, size_t count) {
    size_t nread = 0;
    unsigned char* dest = bytes;
    const unsigned char* src = buf->inner_buf;
    while (nread < count) {
        dest[nread++] = src[buf->read_idx];
        buf->read_idx = (buf->read_idx + 1) % BUF_CAPACITY;
        if (buf->read_idx == buf->write_idx)
            break;
    }
    return nread;
}

ssize_t ring_buf_write(ring_buf* buf, const void* bytes, size_t count) {
    size_t nwritten = 0;
    unsigned char* dest = buf->inner_buf;
    const unsigned char* src = bytes;
    while (nwritten < count) {
        dest[buf->write_idx] = src[nwritten++];
        buf->write_idx = (buf->write_idx + 1) % BUF_CAPACITY;
        if ((buf->write_idx + 1) % BUF_CAPACITY == buf->read_idx)
            break;
    }
    return nwritten;
}

ssize_t ring_buf_write_evicting_oldest(ring_buf* buf, const void* bytes,
                                       size_t count) {
    size_t nwritten = 0;
    unsigned char* dest = buf->inner_buf;
    const unsigned char* src = bytes;
    while (nwritten < count) {
        dest[buf->write_idx] = src[nwritten++];
        buf->write_idx = (buf->write_idx + 1) % BUF_CAPACITY;
    }
    return nwritten;
}
