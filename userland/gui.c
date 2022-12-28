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

unsigned int genbufferpos(int x, int y, int pixelwidth, int pitch) {
    return x * pixelwidth + y * pitch;

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
    struct fb_info framebuffer_info;
    if (ioctl(framebuffer_fd, FBIOGET_INFO, &framebuffer_info) < 0) {
        perror("ioctl");
        close(framebuffer_fd);
        return EXIT_FAILURE;
    }
    void* framebuffer = mmap(NULL, framebuffer_info.pitch * framebuffer_info.height, PROT_READ | PROT_WRITE, MAP_SHARED, framebuffer_fd, 0);
    close(framebuffer_fd);
    printf("Got framebuffer pointer. Address of it is: %d\n", &framebuffer);
    memset(framebuffer, 0, framebuffer_info.pitch * framebuffer_info.height);
    for(int i; i < framebuffer_info.width; i++)
        for(int k; k < framebuffer_info.height; k++)
            ((char*)framebuffer)[genbufferpos(i, k, framebuffer_info.bpp, framebuffer_info.pitch)] = 4;
    getchar();
    return EXIT_SUCCESS;
}
