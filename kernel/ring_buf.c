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

ssize_t ring_buf_write_evicting_oldest(ring_buf* buf, const void* bytes, size_t count) {
    size_t nwritten = 0;
    unsigned char* dest = buf->inner_buf;
    const unsigned char* src = bytes;
    while (nwritten < count) {
        dest[buf->write_idx] = src[nwritten++];
        buf->write_idx = (buf->write_idx + 1) % BUF_CAPACITY;
    }
    return nwritten;
}
