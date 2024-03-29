/*
 *  .OOOOOO.   OOOO                                .    O8O              
 *  D8P'  `Y8B  `888                              .O8    `"'              
 * 888           888 .OO.    .OOOO.    .OOOOO.  .O888OO OOOO  OOOO    OOO 
 * 888           888P"Y88B  `P  )88B  D88' `88B   888   `888   `88B..8P'  
 * 888           888   888   .OP"888  888   888   888    888     Y888'    
 * `88B    OOO   888   888  D8(  888  888   888   888 .  888   .O8"'88B   
 *  `Y8BOOD8P'  O888O O888O `Y888""8O `Y8BOD8P'   "888" O888O O88'   888O 
 * 
 *  Chaotix is a UNIX-like operating system that consists of a kernel written in C and
 *  i?86 assembly, and userland binaries written in C.
 *     
 *  Copyright (c) 2023 Nexuss
 *  Copyright (c) 2022 mosm
 *  Copyright (c) 2006-2018 Frans Kaashoek, Robert Morris, Russ Cox, Massachusetts Institute of Technology
 *
 *  This file may or may not contain code from https://github.com/mosmeh/yagura, and/or
 *  https://github.com/mit-pdos/xv6-public. Both projects have the same license as this
 *  project, and the license can be seen below:
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *  
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *  
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 *  THE SOFTWARE.
 */

#include <errno.h>
#include <fb.h>
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/mman.h>
#include <unistd.h>
#include <escp.h>

#define TGA_HEADER_SIZE 18
#define TGA_DATA_OFFSET 18
#define PSF_FONT_MAGIC 0x864AB572
#define VFB_ARRAY_SIZE 256

/*
 *  We can only have multiple processes running at the same time for now, not multiple threads. Therefore, we have to implement a simple
 *  mechanism for multithreading...
 */
#define THREADS 2

#define RGBA(r, g, b, a) (((a) << 24) | ((r) << 16) | ((g) << 8) | (b))
#define RGB(r, g, b) RGBA(r, g, b, 255U)

/* Define the different instances of the window system... */
enum {
    display
};

/* Contain the instance type of the window system... */
int mode;

/* This struct defines the info for a virtual framebuffer, which is a framebuffer that can be embedded inside a physical framebuffer... */
typedef struct {
    int width;
    int height;
    int x;
    int y;
    int pitch;
    unsigned char* data;
} virt_fb;

struct fb_info _fb_info;
char * fb;
virt_fb vfb_array[VFB_ARRAY_SIZE];

struct psf_header {
    unsigned int magic;
    unsigned int version;
    unsigned int header_size;
    unsigned int flags;
    unsigned int length;
    unsigned int char_size;
    unsigned int height;
    unsigned int width;
};

/*
 *  Convert on-screen coordinates to position in framebuffer.
 */
unsigned int genbufferpos(int x, int y) {
    return x * 4 + y * _fb_info.pitch;
}

/* Initialize a virtual framebuffer... */
void init_vfb(unsigned int vfb_index, unsigned int x, unsigned int y, unsigned int width, unsigned int height) {
    virt_fb *vfb = &vfb_array[vfb_index];
    vfb->x = x;
    vfb->y = y;
    vfb->width = width;
    vfb->height = height;
    vfb->pitch = width * sizeof(unsigned int);
    vfb->data = (unsigned char *) malloc(vfb->pitch * height);
}

/* Plot pixel to a virtual framebuffer; this may later be removed... */
void draw_to_vfb(unsigned int vfb_index, unsigned int x, unsigned int y, unsigned int color) {
    virt_fb *vfb = &vfb_array[vfb_index];
    unsigned int *pixel = (unsigned int *) (vfb->data + y * vfb->pitch + x * sizeof(unsigned int));
    *pixel = color;
}

/* Only draws 8x8 PSF files... */
void draw_text(char *text, int x, int y, char *fb_mem, int fb_width, int fb_height, char *psf_filename, int color) {
    int psf_fd = open(psf_filename, O_RDONLY);
    if (psf_fd < 0) {
        printf("error: could not open PSF font file...\n");
        return;
    }

    struct psf_header psf_header;
    read(psf_fd, &psf_header, sizeof(struct psf_header));

    if (psf_header.magic != PSF_FONT_MAGIC) {
        printf("error: invalid PSF font file...\n");
        close(psf_fd);
        return;
    }

    int char_size = psf_header.char_size;
    char char_buffer[char_size];

    while (*text) {
        lseek(psf_fd, psf_header.header_size + (unsigned int)(*text) * char_size, SEEK_SET);
        read(psf_fd, char_buffer, char_size);

        int i, j, k;
        for (i = 0; i < psf_header.height; i++) {
            for (j = 0; j < psf_header.width; j++) {
                char pixel = char_buffer[i * ((psf_header.width + 7) / 8) + (j / 8)];
                int pixel_value = (pixel >> (7 - j % 8)) & 1;
                if (pixel_value) {
                    unsigned int pos = genbufferpos(x + j, y + i);
                    /* The `4` in the line below represents bytes-per-pixel. */
                    for (k = 0; k < 4; k++) {
                        fb_mem[pos + k] = color;
                    }
                }
            }
        }

        x += psf_header.width;
        text++;
    }

    close(psf_fd);
}

/*
 *  REMEMBER: to convert a PNG file to a TGA file that the function below supports, use ImageMagick to convert it with the following
 *  command:
 *  1 | convert input.png -depth 8 -type truecolor output.tga
 */

void draw_tga(const char *filename, int x, int y, uint8_t *fb, int fb_width) {
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
        return;
    }

    uint8_t header[TGA_HEADER_SIZE];
    if (read(fd, header, TGA_HEADER_SIZE) != TGA_HEADER_SIZE) {
        perror("Error reading TGA header");
        close(fd);
        return;
    }

    int width = header[12] + (header[13] << 8);
    int height = header[14] + (header[15] << 8);
    int bpp = header[16] >> 3;

    if (bpp != 3) {
        printf("Error: only 24-bit RGB TGA files are supported\n");
        close(fd);
        return;
    }

    int image_size = width * height * bpp;
    uint8_t *data = malloc(image_size);

    if (lseek(fd, TGA_DATA_OFFSET, SEEK_SET) == -1) {
        perror("Error seeking to TGA data");
        free(data);
        close(fd);
        return;
    }

    if (read(fd, data, image_size) != image_size) {
        perror("Error reading TGA data");
        free(data);
        close(fd);
        return;
    }

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            int pixel_offset = i * width * bpp + j * bpp;
            int fb_offset = (y + i) * fb_width * 4 + (x + j) * 4;

            fb[fb_offset] = data[pixel_offset + 2];     // Red
            fb[fb_offset + 1] = data[pixel_offset + 1]; // Green
            fb[fb_offset + 2] = data[pixel_offset];     // Blue
            fb[fb_offset + 3] = 0xFF;                   // Alpha
        }
    }

    free(data);
    close(fd);
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

/* Return an index of an unused virtual framebuffer if it is even found. Return `-1` if one can't be found... */
int get_free_vfb_index() {
    for (int i = 0; i < sizeof(vfb_array) / sizeof(virt_fb); i++) {
        if (vfb_array[i].data == NULL) {
            return i;
        }
    }
    return -1;
}

/*
 *  Draw a panel on screen.
 */
void paint_panel(int pos_x, int pos_y, int size_x, int size_y) {
    paint_rect(pos_x, pos_y, size_x, size_y, RGB(200, 200, 200));
    paint_rect(pos_x + 2, pos_y + 2, size_x + 2, size_y + 2, RGB(255, 255, 255));
}

/*
 *  Draw a window. Make sure to provide it a virtual framebuffer index, it's position, size, and title...
 */
int paint_window(int index, int pos_x, int pos_y, int size_x, int size_y, const char *text) {
    int last_pos_x = -1;
    int last_pos_y = -1;
    int last_size_x = -1;
    int last_size_y = -1;
    
    if(pos_x == last_pos_x && pos_y == last_pos_y && size_x == last_size_x && size_y == last_size_y) {
        /* Position or size hasn't changed, therefore let's just draw the virtual framebuffer... */
        goto drawrest;
    }
    
    last_pos_x = pos_x;
    last_pos_y = pos_y;
    last_size_x = size_x;
    last_size_y = size_y;
    
    paint_rect(pos_x, pos_y, size_x, size_y, RGB(200, 200, 200));
    paint_rect(pos_x + 2, pos_y + 2, size_x + 2, size_y + 2, RGB(255, 255, 255));
    paint_rect(pos_x + 4, pos_y + 4, size_x - 2, 18, RGB(0, 0, 150));
    draw_text(text, pos_x + 10, pos_y + 10, fb, _fb_info.width, _fb_info.height, "/usr/share/fonts/Anikki-8x8.psf", RGB(255, 255, 255));
    draw_tga("/usr/share/bitmaps/close_up.tga", pos_x + size_x - 14, pos_y + 6, fb, _fb_info.width);
    draw_tga("/usr/share/bitmaps/maximize_up.tga", pos_x + size_x - 31, pos_y + 6, fb, _fb_info.width);
    draw_tga("/usr/share/bitmaps/minimize_up.tga", pos_x + size_x - 48, pos_y + 6, fb, _fb_info.width);

drawrest:

    init_vfb(index, pos_x + 4, pos_y + 25, size_x - 1, size_y - 22);
    copy_to_physfb(index);
    return index;
}

/* Actually display virtual framebuffer contents to physical framebuffer... */
void copy_to_physfb(uint32_t vfb_index) {
    virt_fb *vfb = &vfb_array[vfb_index];
    for (int y = 0; y < vfb->height; y++) {
        for (int x = 0; x < vfb->width; x++) {
            uint32_t *virt_pixel = (uint32_t *)(vfb->data + y * vfb->pitch + x * sizeof(uint32_t));
            uint32_t *phys_pixel = (uint32_t *)(fb + genbufferpos(vfb->x + x, vfb->y + y));
            *phys_pixel = *virt_pixel;
        }
    }
}

/* Move and/or resize virtual framebuffer*/
void translate_vfb(uint32_t vfb_index, uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
    virt_fb *vfb = &vfb_array[vfb_index];
    
    /* De-allocate old virtual framebuffer's data... r*/
    free(vfb->data);
    vfb->x = x;
    vfb->y = y;
    vfb->width = width;
    vfb->height = height;

    /* Calculate new pitch and allocate memory for new virtual framebuffer's memory... */
    vfb->pitch = width * sizeof(uint32_t);
    vfb->data = (uint8_t *) malloc(vfb->pitch * height);
}

int main() {
    if(fork() == 0) {
        mode = display;
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
        int index = get_free_vfb_index();
        if(index == -1) {
            return -1;
        }
        paint_window(index, 25, 25, 400, 100, "Hello, world!");

        for(int x = 0; x <= 10; x++) {
            for(int y = 0; y <= 10; y++) {
                draw_to_vfb(index, x, y, RGB(255, 255, 255));
            }
        }
        copy_to_physfb(index);
    }

    getchar();
    return EXIT_SUCCESS;
}
