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
#include "window.h"

#define MAX_WINDOWS 1024

int main(void) {
    struct window_properties window;
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

    window.enabled = 1;
    window.x = 25;
    window.y = 25;
    window.height = 100;
    window.width = 200;
    strcpy(window.title, "Sample Window!");

    /*for(int x = 0; x < 100; x++)
    {
        for(int y = 0; y < 100; y++)
        {
            struct pixel_info object;
            object.x = x;
            object.y = y;
            object.r = 200;
            object.g = 200;
            object.b = 200;
            draw_pixel(object, framebuffer_info, framebuffer);
        }
    }*/

    if(fork() == 0)
    {
        while(1)
        {
            memset (framebuffer, 0, framebuffer_info.width * framebuffer_info.height * (framebuffer_info.bpp / 8));
            for(int i = 0; i <= MAX_WINDOWS; i++)
            {
                if(window.enabled == 1)
                {
                    for(int x = window.x; x <= window.x + window.width; x++)
                    {
                        for(int y = window.y; y <= window.y + window.width; y++)
                        {
                            struct pixel_info pixel;
                            pixel.x = x;
                            pixel.y = y;
                            pixel.r = 200;
                            pixel.g = 200;
                            pixel.b = 200;
                            draw_pixel(pixel, framebuffer_info, framebuffer);
                        }
                    }
                }
            }
        }
    }

    while(1)
    ;

    close(framebuffer_fd);
    return EXIT_SUCCESS;
}
