#pragma once

#include "fs/fs.h"
#include "ring_buf.h"

typedef struct unix_socket {
    struct inode inode;
    int backlog;

    mutex pending_queue_lock;
    atomic_size_t num_pending;
    struct unix_socket* next; // pending queue

    atomic_bool connected;
    file_description* connector_fd;

    ring_buf server_to_client_buf;
    ring_buf client_to_server_buf;
} unix_socket;

unix_socket* unix_socket_create(void);
void unix_socket_set_backlog(unix_socket*, int backlog);
NODISCARD unix_socket* unix_socket_accept(unix_socket* listener);
NODISCARD int unix_socket_connect(file_description* connector_fd,
                                  unix_socket* listener);
