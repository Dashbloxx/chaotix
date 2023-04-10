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

#include "fs.h"
#include <kernel/api/fcntl.h>
#include <kernel/api/signum.h>
#include <kernel/panic.h>
#include <kernel/process.h>
#include <kernel/ring_buf.h>

#define BUF_CAPACITY 1024

struct fifo {
    struct inode inode;
    ring_buf buf;
    atomic_size_t num_readers;
    atomic_size_t num_writers;
};

static void fifo_destroy_inode(struct inode* inode) {
    struct fifo* fifo = (struct fifo*)inode;
    ring_buf_destroy(&fifo->buf);
    kfree(fifo);
}

static int fifo_open(file_description* desc, int flags, mode_t mode) {
    (void)flags;
    (void)mode;
    if ((flags & O_RDONLY) && (flags & O_WRONLY))
        return -EINVAL;
    struct fifo* fifo = (struct fifo*)desc->inode;
    if (flags & O_RDONLY)
        ++fifo->num_readers;
    if (flags & O_WRONLY)
        ++fifo->num_writers;
    return 0;
}

static int fifo_close(file_description* desc) {
    ASSERT(!((desc->flags & O_RDONLY) && (desc->flags & O_WRONLY)));
    struct fifo* fifo = (struct fifo*)desc->inode;
    if (desc->flags & O_RDONLY)
        --fifo->num_readers;
    if (desc->flags & O_WRONLY)
        --fifo->num_writers;
    return 0;
}

static bool read_should_unblock(file_description* desc) {
    const struct fifo* fifo = (const struct fifo*)desc->inode;
    return fifo->num_writers == 0 || !ring_buf_is_empty(&fifo->buf);
}

static ssize_t fifo_read(file_description* desc, void* buffer, size_t count) {
    struct fifo* fifo = (struct fifo*)desc->inode;
    ring_buf* buf = &fifo->buf;

    for (;;) {
        int rc = file_description_block(desc, read_should_unblock);
        if (IS_ERR(rc))
            return rc;

        mutex_lock(&buf->lock);
        if (!ring_buf_is_empty(buf)) {
            ssize_t nread = ring_buf_read(buf, buffer, count);
            mutex_unlock(&buf->lock);
            return nread;
        }

        bool no_writer = fifo->num_writers == 0;
        mutex_unlock(&buf->lock);
        if (no_writer)
            return 0;
    }
}

static bool write_should_unblock(struct file_description* desc) {
    struct fifo* fifo = (struct fifo*)desc->inode;
    return fifo->num_readers == 0 || !ring_buf_is_full(&fifo->buf);
}

static ssize_t fifo_write(file_description* desc, const void* buffer,
                          size_t count) {
    struct fifo* fifo = (struct fifo*)desc->inode;
    ring_buf* buf = &fifo->buf;

    for (;;) {
        int rc = file_description_block(desc, write_should_unblock);
        if (IS_ERR(rc))
            return rc;

        mutex_lock(&buf->lock);
        if (fifo->num_readers == 0) {
            mutex_unlock(&buf->lock);
            int rc = process_send_signal_to_one(current->pid, SIGPIPE);
            if (IS_ERR(rc))
                return rc;
            return -EPIPE;
        }

        if (ring_buf_is_full(buf)) {
            mutex_unlock(&buf->lock);
            continue;
        }

        ssize_t nwritten = ring_buf_write(buf, buffer, count);
        mutex_unlock(&buf->lock);
        return nwritten;
    }
}

struct inode* fifo_create(void) {
    struct fifo* fifo = kmalloc(sizeof(struct fifo));
    if (!fifo)
        return ERR_PTR(-ENOMEM);
    *fifo = (struct fifo){0};

    int rc = ring_buf_init(&fifo->buf);
    if (IS_ERR(rc))
        return ERR_PTR(rc);

    struct inode* inode = &fifo->inode;
    static file_ops fops = {.destroy_inode = fifo_destroy_inode,
                            .open = fifo_open,
                            .close = fifo_close,
                            .read = fifo_read,
                            .write = fifo_write};
    inode->fops = &fops;
    inode->mode = S_IFIFO;
    inode->ref_count = 1;

    return (struct inode*)fifo;
}
