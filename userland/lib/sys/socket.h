#pragma once

#include <kernel/api/sys/socket.h>

int socket(int domain, int type, int protocol);
int bind(int sockfd, const sockaddr* addr, socklen_t addrlen);
int listen(int sockfd, int backlog);
int accept(int sockfd, struct sockaddr* addr, socklen_t* addrlen);
int connect(int sockfd, const struct sockaddr* addr, socklen_t addrlen);
