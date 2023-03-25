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
 *  Parse TGA bitmap, and return a pointer to an unsigned int array (This function is from OSDev.org).
 */
unsigned int *tga_parse(unsigned char *ptr, int size)
{
    unsigned int *data;
    int i, j, k, x, y, w = (ptr[13] << 8) + ptr[12], h = (ptr[15] << 8) + ptr[14], o = (ptr[11] << 8) + ptr[10];
    int m = ((ptr[1]? (ptr[7]>>3)*ptr[5] : 0) + 18);
 
    if(w<1 || h<1) return NULL;
 
    data = (unsigned int*)malloc((w*h+2)*sizeof(unsigned int));
    if(!data) return NULL;
 
    switch(ptr[2]) {
        case 1:
            if(ptr[6]!=0 || ptr[4]!=0 || ptr[3]!=0 || (ptr[7]!=24 && ptr[7]!=32)) { free(data); return NULL; }
            for(y=i=0; y<h; y++) {
                k = ((!o?h-y-1:y)*w);
                for(x=0; x<w; x++) {
                    j = ptr[m + k++]*(ptr[7]>>3) + 18;
                    data[2 + i++] = ((ptr[7]==32?ptr[j+3]:0xFF) << 24) | (ptr[j+2] << 16) | (ptr[j+1] << 8) | ptr[j];
                }
            }
            break;
        case 2:
            if(ptr[5]!=0 || ptr[6]!=0 || ptr[1]!=0 || (ptr[16]!=24 && ptr[16]!=32)) { free(data); return NULL; }
            for(y=i=0; y<h; y++) {
                j = ((!o?h-y-1:y)*w*(ptr[16]>>3));
                for(x=0; x<w; x++) {
                    data[2 + i++] = ((ptr[16]==32?ptr[j+3]:0xFF) << 24) | (ptr[j+2] << 16) | (ptr[j+1] << 8) | ptr[j];
                    j += ptr[16]>>3;
                }
            }
            break;
        case 9:
            if(ptr[6]!=0 || ptr[4]!=0 || ptr[3]!=0 || (ptr[7]!=24 && ptr[7]!=32)) { free(data); return NULL; }
            y = i = 0;
            for(x=0; x<w*h && m<size;) {
                k = ptr[m++];
                if(k > 127) {
                    k -= 127; x += k;
                    j = ptr[m++]*(ptr[7]>>3) + 18;
                    while(k--) {
                        if(!(i%w)) { i=((!o?h-y-1:y)*w); y++; }
                        data[2 + i++] = ((ptr[7]==32?ptr[j+3]:0xFF) << 24) | (ptr[j+2] << 16) | (ptr[j+1] << 8) | ptr[j];
                    }
                } else {
                    k++; x += k;
                    while(k--) {
                        j = ptr[m++]*(ptr[7]>>3) + 18;
                        if(!(i%w)) { i=((!o?h-y-1:y)*w); y++; }
                        data[2 + i++] = ((ptr[7]==32?ptr[j+3]:0xFF) << 24) | (ptr[j+2] << 16) | (ptr[j+1] << 8) | ptr[j];
                    }
                }
            }
            break;
        case 10:
            if(ptr[5]!=0 || ptr[6]!=0 || ptr[1]!=0 || (ptr[16]!=24 && ptr[16]!=32)) { free(data); return NULL; }
            y = i = 0;
            for(x=0; x<w*h && m<size;) {
                k = ptr[m++];
                if(k > 127) {
                    k -= 127; x += k;
                    while(k--) {
                        if(!(i%w)) { i=((!o?h-y-1:y)*w); y++; }
                        data[2 + i++] = ((ptr[16]==32?ptr[m+3]:0xFF) << 24) | (ptr[m+2] << 16) | (ptr[m+1] << 8) | ptr[m];
                    }
                    m += ptr[16]>>3;
                } else {
                    k++; x += k;
                    while(k--) {
                        if(!(i%w)) { i=((!o?h-y-1:y)*w); y++; }
                        data[2 + i++] = ((ptr[16]==32?ptr[m+3]:0xFF) << 24) | (ptr[m+2] << 16) | (ptr[m+1] << 8) | ptr[m];
                        m += ptr[16]>>3;
                    }
                }
            }
            break;
        default:
            free(data); return NULL;
    }
    data[0] = w;
    data[1] = h;
    return data;
}

/*
 *  Convert on-screen coordinates to position in framebuffer.
 */
unsigned int genbufferpos(int x, int y) {
    return x * 4 + y * _fb_info.pitch;
}

/*
 *  Draws pixels to screen. These pixels can be the pixels returned by
 */
void paint_pixels(int x, int y, unsigned int *pixels) {
    int width = pixels[0];
    int height = pixels[1];
    /*
     *  Let's start off from here tomorrow, my head is hurting...
     */
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

    getchar();
    return EXIT_SUCCESS;
}
