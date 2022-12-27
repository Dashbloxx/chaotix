#include <kernel/api/err.h>
#include <kernel/api/fcntl.h>
#include <kernel/api/sys/socket.h>
#include <kernel/process.h>
#include <kernel/socket.h>

int sys_socket(int domain, int type, int protocol) {
    (void)protocol;
    if (domain != AF_UNIX || type != SOCK_STREAM)
        return -EAFNOSUPPORT;

    unix_socket* socket = unix_socket_create();
    if (IS_ERR(socket))
        return PTR_ERR(socket);
    inode_ref((struct inode*)socket);
    file_description* desc = inode_open((struct inode*)socket, O_RDWR, 0);
    if (IS_ERR(desc))
        return PTR_ERR(desc);
    return process_alloc_file_descriptor(-1, desc);
}

int sys_bind(int sockfd, const sockaddr* addr, socklen_t addrlen) {
    file_description* desc = process_get_file_description(sockfd);
    if (IS_ERR(desc))
        return PTR_ERR(desc);
    if (!S_ISSOCK(desc->inode->mode))
        return -ENOTSOCK;
    unix_socket* socket = (unix_socket*)desc->inode;

    if (addrlen <= sizeof(sa_family_t) || sizeof(sockaddr_un) < addrlen)
        return -EINVAL;
    const sockaddr_un* addr_un = (const sockaddr_un*)addr;
    if (addr->sa_family != AF_UNIX)
        return -EINVAL;

    const char* path =
        kstrndup(addr_un->sun_path, addrlen - offsetof(sockaddr_un, sun_path));
    if (!path)
        return -ENOMEM;

    file_description* bound_desc = vfs_open(path, O_CREAT | O_EXCL, S_IFSOCK);
    if (IS_ERR(bound_desc)) {
        if (PTR_ERR(bound_desc) == -EEXIST)
            return -EADDRINUSE;
        return PTR_ERR(bound_desc);
    }
    bound_desc->inode->bound_socket = socket;
    return 0;
}

int sys_listen(int sockfd, int backlog) {
    file_description* desc = process_get_file_description(sockfd);
    if (IS_ERR(desc))
        return PTR_ERR(desc);
    if (!S_ISSOCK(desc->inode->mode))
        return -ENOTSOCK;

    unix_socket* socket = (unix_socket*)desc->inode;
    unix_socket_set_backlog(socket, backlog);
    return 0;
}

int sys_accept(int sockfd, struct sockaddr* addr, socklen_t* addrlen) {
    file_description* desc = process_get_file_description(sockfd);
    if (IS_ERR(desc))
        return PTR_ERR(desc);
    if (!S_ISSOCK(desc->inode->mode))
        return -ENOTSOCK;

    unix_socket* listener = (unix_socket*)desc->inode;
    unix_socket* connector = unix_socket_accept(listener);
    if (IS_ERR(connector))
        return PTR_ERR(connector);
    inode_ref((struct inode*)connector);
    file_description* connector_desc =
        inode_open((struct inode*)connector, O_RDWR, 0);
    if (IS_ERR(connector_desc))
        return PTR_ERR(connector_desc);

    int fd = process_alloc_file_descriptor(-1, connector_desc);
    if (IS_ERR(fd))
        return fd;

    if (addr) {
        sockaddr_un* addr_un = (sockaddr_un*)addr;
        addr_un->sun_family = AF_UNIX;
        addr_un->sun_path[0] = 0;
    }
    if (addrlen)
        *addrlen = sizeof(sockaddr_un);

    return fd;
}

int sys_connect(int sockfd, const struct sockaddr* addr, socklen_t addrlen) {
    file_description* desc = process_get_file_description(sockfd);
    if (IS_ERR(desc))
        return PTR_ERR(desc);
    if (!S_ISSOCK(desc->inode->mode))
        return -ENOTSOCK;
    unix_socket* socket = (unix_socket*)desc->inode;
    if (socket->connected)
        return -EISCONN;

    if (addrlen <= sizeof(sa_family_t) || sizeof(sockaddr_un) < addrlen)
        return -EINVAL;
    const sockaddr_un* addr_un = (const sockaddr_un*)addr;
    if (addr->sa_family != AF_UNIX)
        return -EINVAL;
    const char* path =
        kstrndup(addr_un->sun_path, addrlen - offsetof(sockaddr_un, sun_path));
    if (!path)
        return -ENOMEM;
    file_description* listener_desc = vfs_open(path, 0, 0);
    if (IS_ERR(listener_desc))
        return PTR_ERR(listener_desc);
    unix_socket* listener = listener_desc->inode->bound_socket;
    if (!listener)
        return -ECONNREFUSED;

    return unix_socket_connect(desc, listener);
}
