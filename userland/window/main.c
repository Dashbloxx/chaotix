#include <errno.h>
#include <extra.h>
#include <fb.h>
#include <fcntl.h>
#include <panic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "draw.h"

int main(void) {
    int framebuffer_fd = open("/dev/fb0", O_RDWR);
    if (framebuffer_fd < 0) {
        if (errno == ENOENT)
            dprintf(STDERR_FILENO, "Framebuffer is not available\n");
        else
            perror("open");
        return EXIT_FAILURE;
    }
    struct fb_info framebuffer_info;
    if (ioctl(framebuffer_fd, FBIOGET_INFO, &framebuffer_info) < 0) {
        perror("ioctl");
        close(framebuffer_fd);
        return EXIT_FAILURE;
    }
    void * framebuffer = mmap(NULL, framebuffer_info.pitch * framebuffer_info.height, PROT_READ | PROT_WRITE, MAP_SHARED, framebuffer_fd, 0);
    
    for(int x = 0; x < 100; x++)
    {
        for(int y = 0; y < 100; y++)
        {
            struct draw_info object;
            object.x = x;
            object.y = y;
            object.r = 255;
            object.g = 0;
            object.b = 255;
            draw_pixel(object, framebuffer_info, framebuffer);
        }
    }

    while(1)
    ;

    close(framebuffer_fd);
    return EXIT_SUCCESS;
}
