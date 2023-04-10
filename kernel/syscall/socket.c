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
