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

#include "memory/memory.h"
#include "panic.h"
#include "scheduler.h"
#include "socket.h"

static void unix_socket_destroy_inode(struct inode* inode) {
    unix_socket* socket = (unix_socket*)inode;
    ring_buf_destroy(&socket->server_to_client_buf);
    ring_buf_destroy(&socket->client_to_server_buf);
    kfree(socket);
}

static ring_buf* get_buf_to_read(unix_socket* socket, file_description* desc) {
    bool is_client = socket->connector_fd == desc;
    return is_client ? &socket->server_to_client_buf : &socket->client_to_server_buf;
}

static ring_buf* get_buf_to_write(unix_socket* socket, file_description* desc) {
    bool is_client = socket->connector_fd == desc;
    return is_client ? &socket->client_to_server_buf : &socket->server_to_client_buf;
}

static bool read_should_unblock(file_description* desc) {
    unix_socket* socket = (unix_socket*)desc->inode;
    ring_buf* buf = get_buf_to_read(socket, desc);
    return !ring_buf_is_empty(buf);
}

static ssize_t unix_socket_read(file_description* desc, void* buffer, size_t count) {
    unix_socket* socket = (unix_socket*)desc->inode;
    ring_buf* buf = get_buf_to_read(socket, desc);

    for (;;) {
        int rc = file_description_block(desc, read_should_unblock);
        if (IS_ERR(rc))
            return rc;

        mutex_lock(&buf->lock);
        if (ring_buf_is_empty(buf)) {
            mutex_unlock(&buf->lock);
            continue;
        }
        ssize_t nread = ring_buf_read(buf, buffer, count);
        mutex_unlock(&buf->lock);
        return nread;
    }
}

static bool write_should_unblock(file_description* desc) {
    unix_socket* socket = (unix_socket*)desc->inode;
    ring_buf* buf = get_buf_to_write(socket, desc);
    return !ring_buf_is_full(buf);
}

static ssize_t unix_socket_write(file_description* desc, const void* buffer, size_t count) {
    unix_socket* socket = (unix_socket*)desc->inode;
    ring_buf* buf = get_buf_to_write(socket, desc);

    for (;;) {
        int rc = file_description_block(desc, write_should_unblock);
        if (IS_ERR(rc))
            return rc;

        mutex_lock(&buf->lock);
        if (ring_buf_is_full(buf)) {
            mutex_unlock(&buf->lock);
            continue;
        }
        ssize_t nwritten = ring_buf_write(buf, buffer, count);
        mutex_unlock(&buf->lock);
        return nwritten;
    }
}

unix_socket* unix_socket_create(void) {
    unix_socket* socket = kmalloc(sizeof(unix_socket));
    if (!socket)
        return ERR_PTR(-ENOMEM);
    *socket = (unix_socket){0};

    struct inode* inode = &socket->inode;
    static file_ops fops = {.destroy_inode = unix_socket_destroy_inode, .read = unix_socket_read, .write = unix_socket_write};
    inode->fops = &fops;
    inode->mode = S_IFSOCK;
    inode->ref_count = 1;

    int rc = ring_buf_init(&socket->client_to_server_buf);
    if (IS_ERR(rc))
        return ERR_PTR(rc);
    rc = ring_buf_init(&socket->server_to_client_buf);
    if (IS_ERR(rc))
        return ERR_PTR(rc);

    return socket;
}

void unix_socket_set_backlog(unix_socket* socket, int backlog) {
    socket->backlog = backlog;
}

static void enqueue_pending(unix_socket* listener, unix_socket* connector) {
    connector->next = NULL;
    mutex_lock(&listener->pending_queue_lock);

    if (listener->next) {
        unix_socket* it = listener->next;
        while (it->next)
            it = it->next;
        it->next = connector;
    } else {
        listener->next = connector;
    }

    mutex_unlock(&listener->pending_queue_lock);
    ++listener->num_pending;
}

static unix_socket* deque_pending(unix_socket* listener) {
    mutex_lock(&listener->pending_queue_lock);

    unix_socket* connector = listener->next;
    listener->next = connector->next;

    mutex_unlock(&listener->pending_queue_lock);
    --listener->num_pending;

    ASSERT(connector);
    return connector;
}

static bool accept_should_unblock(atomic_size_t* num_pending) {
    return *num_pending > 0;
}

unix_socket* unix_socket_accept(unix_socket* listener) {
    int rc = scheduler_block((should_unblock_fn)accept_should_unblock, &listener->num_pending);
    if (IS_ERR(rc))
        return ERR_PTR(rc);

    unix_socket* connector = deque_pending(listener);
    ASSERT(!connector->connected);
    connector->connected = true;
    return connector;
}

static bool connect_should_unblock(atomic_bool* connected) {
    return *connected;
}

int unix_socket_connect(file_description* connector_fd, unix_socket* listener) {
    unix_socket* connector = (unix_socket*)connector_fd->inode;
    connector->connector_fd = connector_fd;

    if (listener->num_pending >= (size_t)listener->backlog)
        return -ECONNREFUSED;
    enqueue_pending(listener, connector);

    return scheduler_block((should_unblock_fn)connect_should_unblock, &connector->connected);
}
