#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

/* This is where the window server & window client will communicate. They both use UNIX sockets to communicate, by the way... */
#define SOCKET_PATH "/tmp/window"

/* These structs below are to be used inside both the window server & the window client. */

typedef struct {
    /* The title of the window that is to be displayed... */
    char *title;
    /* The ID of the window that is to be displayed... */
    int id;
    /* The virtual framebuffer of the window... */
    void *fb;
} window_t;

typedef struct {
    /* Multiple amounts of windows... */
    window_t *windows;
    char *communication_socket;
} context_t;

int main() {
    struct sockaddr_un addr;
    return 0;
}