#include <errno.h>
#include <fb.h>
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

struct fb_info _fb_info;
char * fb;

unsigned int genbufferpos(int x, int y) {
    return x * 4 + y * _fb_info.pitch;
}

int main() {
    /*
     *  Here we open the framebuffer character device, and get a pointer to the framebuffer. After getting a pointer to the framebuffer, we simply close the
     *  the framebuffer character device, and start using the framebuffer pointer to draw to the framebuffer.
     */
    int fd = open("/dev/fb0", O_RDWR);
    if (fd < 0) {
        if (errno == ENOENT)
            dprintf(STDERR_FILENO, "Framebuffer is not available\n");
        else
            perror("open");
        return EXIT_FAILURE;
    }
    if (ioctl(fd, FBIOGET_INFO, &_fb_info) < 0) {
        perror("ioctl");
        return EXIT_FAILURE;
    }
    fb = mmap(NULL, _fb_info.pitch * _fb_info.height, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (fb == MAP_FAILED) {
        perror("mmap");
        return EXIT_FAILURE;
    }
    close(fd);

    /*
     *  Now we can actually use `fb` to draw to the framebuffer and do much more!
     */
    for(int x = 0; x <= 50; x++)
    {
        for(int y = 0; y <= 50; y++)
        {
            *(fb + genbufferpos(x, y)) = 100;
        }
    }

    return EXIT_SUCCESS;
}
