#pragma once

#include <stdint.h>

#define AF_UNIX 1
#define AF_LOCAL AF_UNIX

#define SOCK_STREAM 1

typedef uint16_t sa_family_t;
typedef uint32_t socklen_t;

typedef struct sockaddr {
    sa_family_t sa_family;
    char sa_data[14];
} sockaddr;

typedef struct sockaddr_un {
    sa_family_t sun_family;
    char sun_path[108];
} sockaddr_un;
