#include <errno.h>
#include <fb.h>
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <escp.h>

#define RGBA(r, g, b, a) (((a) << 24) | ((r) << 16) | ((g) << 8) | (b))
#define RGB(r, g, b) RGBA(r, g, b, 255U)

struct fb_info _fb_info;
char * fb;

/*
 *  A blank button that can be used as a template for exit button, maximize button, and minimize button. If you see `-1`, that just means create a new line...
 *  We drew this on www.pixilart.com, and then we wrote the color of each pixel down here!
 */
int icon0[] = {
    RGB(255, 255, 255), RGB(255, 255, 255), RGB(255, 255, 255), RGB(255, 255, 255), RGB(255, 255, 255), RGB(255, 255, 255), RGB(255, 255, 255), RGB(255, 255, 255), RGB(255, 255, 255), RGB(255, 255, 255), RGB(255, 255, 255), RGB(255, 255, 255), RGB(255, 255, 255), RGB(255, 255, 255), RGB(255, 255, 255), RGB(46, 46, 46), -1,
    RGB(255, 255, 255), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(97, 97, 97), RGB(46, 46, 46), -1,
    RGB(255, 255, 255), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(97, 97, 97), RGB(46, 46, 46), -1,
    RGB(255, 255, 255), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(97, 97, 97), RGB(46, 46, 46), -1,
    RGB(255, 255, 255), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(97, 97, 97), RGB(46, 46, 46), -1,
    RGB(255, 255, 255), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(97, 97, 97), RGB(46, 46, 46), -1,
    RGB(255, 255, 255), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(97, 97, 97), RGB(46, 46, 46), -1,
    RGB(255, 255, 255), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(97, 97, 97), RGB(46, 46, 46), -1,
    RGB(255, 255, 255), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(97, 97, 97), RGB(46, 46, 46), -1,
    RGB(255, 255, 255), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(97, 97, 97), RGB(46, 46, 46), -1,
    RGB(255, 255, 255), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(97, 97, 97), RGB(46, 46, 46), -1,
    RGB(255, 255, 255), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(97, 97, 97), RGB(46, 46, 46), -1,
    RGB(255, 255, 255), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(97, 97, 97), RGB(46, 46, 46), -1,
    RGB(255, 255, 255), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(153, 153, 153), RGB(97, 97, 97), RGB(46, 46, 46), -1,
    RGB(255, 255, 255), RGB(97, 97, 97), RGB(97, 97, 97), RGB(97, 97, 97), RGB(97, 97, 97), RGB(97, 97, 97), RGB(97, 97, 97), RGB(97, 97, 97), RGB(97, 97, 97), RGB(97, 97, 97), RGB(97, 97, 97), RGB(97, 97, 97), RGB(97, 97, 97), RGB(97, 97, 97), RGB(97, 97, 97), RGB(46, 46, 46), -1,
    RGB(46, 46, 46), RGB(46, 46, 46), RGB(46, 46, 46), RGB(46, 46, 46), RGB(46, 46, 46), RGB(46, 46, 46), RGB(46, 46, 46), RGB(46, 46, 46), RGB(46, 46, 46), RGB(46, 46, 46), RGB(46, 46, 46), RGB(46, 46, 46), RGB(46, 46, 46), RGB(46, 46, 46), RGB(46, 46, 46), RGB(46, 46, 46), -1
};

/*
 *  Convert on-screen coordinates to position in framebuffer.
 */
unsigned int genbufferpos(int x, int y) {
    return x * 4 + y * _fb_info.pitch;
}

/*
 *  Draws pixels to screen. These pixels can be the pixels returned by
 */
void paint_pixels(int x, int y, int *image, int size) {
    int width = x;
    int height = y;
    for(int i = 0; i < size; i++) {
        if(image[i] != -1) {
            /* Looks like we're just drawing a pixel here! */
            /* Let's draw the pixel... */
asdasd:
            *(fb + genbufferpos(width, height)) = image[i] & 255;
            *(fb + genbufferpos(width, height) + 1) = (image[i] >> 8) & 255;
            *(fb + genbufferpos(width, height) + 2) = (image[i] >> 16) & 255;
            width++;
        }
        else {
            /* Looks like we're returning to a new row! */
            width = x;
            //height++;
            goto asdasd;
        }
    }
}

/*
 *  Draw a rectangle.
 */
void paint_rect(int pos_x, int pos_y, int size_x, int size_y, int color) {
    for(int x = pos_x; x <= pos_x + size_x; x++)
    {
        for(int y = pos_y; y <= pos_y + size_y; y++)
        {
            *(fb + genbufferpos(x, y)) = color & 255;
            *(fb + genbufferpos(x, y) + 1) = (color >> 8) & 255;
            *(fb + genbufferpos(x, y) + 2) = (color >> 16) & 255;
        }
    }
}

/*
 *  Draw a panel on screen.
 */
void paint_panel(int pos_x, int pos_y, int size_x, int size_y) {
    paint_rect(pos_x, pos_y, size_x, size_y, RGB(200, 200, 200));
    paint_rect(pos_x + 2, pos_y + 2, size_x + 2, size_y + 2, RGB(255, 255, 255));
}

/*
 *  Draw a window.
 */
void paint_window(int pos_x, int pos_y, int size_x, int size_y) {
    paint_rect(pos_x, pos_y, size_x, size_y, RGB(200, 200, 200));
    paint_rect(pos_x + 2, pos_y + 2, size_x + 2, size_y + 2, RGB(255, 255, 255));
    paint_rect(pos_x + 4, pos_y + 4, size_x - 2, 20, RGB(0, 0, 150));
}

int main() {
    /*
     *  Here we open the framebuffer character device, and get a pointer to the framebuffer. After getting a pointer to the framebuffer, we simply close the
     *  the framebuffer character device, and start using the framebuffer pointer to draw to the framebuffer.
     */
    int fd = open("/dev/fb0", O_RDWR);
    if (fd < 0) {
        if (errno == ENOENT)
            dprintf(STDERR_FILENO, "%serror: %sframebuffer is not available%s\n", F_RED, F_YELLOW, RESET);
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
    paint_window(25, 25, 400, 100);

    paint_pixels(100, 100, icon0, sizeof(icon0) / sizeof(int));

    getchar();
    return EXIT_SUCCESS;
}
