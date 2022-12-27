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

static void putpixel(unsigned char* screen, int x, int y, int color, int pixelwidth, int pitch) {
    unsigned where = x * pixelwidth + y * pitch;
    screen[where] = color & 255;
    screen[where + 1] = (color >> 8) & 255;
    screen[where + 2] = (color >> 16) & 255;
}

int main(int argc, char* argv[]) {
    printf("Getting framebuffer address...\n");
    int framebuffer_fd = open("/dev/fb0", O_RDWR);
    if (framebuffer_fd < 0) {
        if (errno == ENOENT)
            dprintf(STDERR_FILENO, "Framebuffer is not available\n");
        else
            perror("open");
        return EXIT_FAILURE;
    }
    struct fb_info fb_info;
    if (ioctl(framebuffer_fd, FBIOGET_INFO, &fb_info) < 0) {
        perror("ioctl");
        close(framebuffer_fd);
        return EXIT_FAILURE;
    }
    void* framebuffer = mmap(NULL, fb_info.pitch * fb_info.height, PROT_READ | PROT_WRITE, MAP_SHARED, framebuffer_fd, 0);
    close(framebuffer_fd);
    printf("Got framebuffer pointer. Address of it is: %d\n", &framebuffer);
    return EXIT_SUCCESS;
}
