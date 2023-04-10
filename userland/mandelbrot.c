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
#include <sys/mman.h>
#include <unistd.h>

#define MAX_ITER 100
#define VIEW_WIDTH 4.5
#define CENTER_X (-0.75)
#define CENTER_Y 0.0

#define LN2 0.693147180559945309417

static double mandelbrot(double x0, double y0) {
    double x = 0;
    double y = 0;
    double x2 = 0;
    double y2 = 0;
    size_t iter = 0;

    for (; x2 + y2 <= 4; ++iter) {
        y = 2 * x * y + y0;
        x = x2 - y2 + x0;
        x2 = x * x;
        y2 = y * y;
        if (iter >= MAX_ITER)
            return iter;
    }

    double lz = sqrt(x * x + y * y) / 2;
    return 1 + iter + log2(lz / LN2);
}

static uint32_t calc_pixel_value(double x0, double y0) {
    double iter = mandelbrot(x0, y0);
    if (iter >= MAX_ITER)
        return 0;

    double hue;
    {
        unsigned int_part = (unsigned)iter;
        double frac_part = iter - int_part;
        double hue1 = (double)int_part / MAX_ITER;
        double hue2 = (int_part + 1.0) / MAX_ITER;
        if (hue1 >= 1)
            hue1 = 0;
        if (hue2 >= 1)
            hue2 = 0;
        hue = hue1 + frac_part * (hue2 - hue1);
    }

    double r = 1;
    double g = 1;
    double b = 1;
    {
        double h = hue * 6;
        unsigned int_part = (unsigned)h;
        double frac_part = h - int_part;
        switch (int_part) {
        case 1:
            r *= 1 - frac_part;
            b = 0;
            break;
        case 2:
            r = 0;
            b *= frac_part;
            break;
        case 3:
            r = 0;
            g *= 1 - frac_part;
            break;
        case 4:
            r *= frac_part;
            g = 0;
            break;
        case 5:
            g = 0;
            b *= 1 - frac_part;
            break;
        case 0:
        default:
            g *= frac_part;
            b = 0;
            break;
        }
    }

    return ((uint8_t)(r * 255) << 16) | ((uint8_t)(g * 255) << 8) |
           (uint8_t)(b * 255);
}

int main(void) {
    int fd = open("/dev/fb0", O_RDWR);
    if (fd < 0) {
        if (errno == ENOENT)
            dprintf(STDERR_FILENO, "Framebuffer is not available\n");
        else
            perror("open");
        return EXIT_FAILURE;
    }
    struct fb_info fb_info;
    if (ioctl(fd, FBIOGET_INFO, &fb_info) < 0) {
        perror("ioctl");
        return EXIT_FAILURE;
    }
    void* fb = mmap(NULL, fb_info.pitch * fb_info.height,
                    PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (fb == MAP_FAILED) {
        perror("mmap");
        return EXIT_FAILURE;
    }
    close(fd);

    double width = VIEW_WIDTH;
    double height = width * fb_info.height / fb_info.width;
    double left = (CENTER_X - width) / 2;
    double top = (CENTER_Y - height) / 2;

    uintptr_t row_addr = (uintptr_t)fb;
    for (size_t y = 0; y < fb_info.height; ++y) {
        double y0 = y * height / fb_info.height + top;

        uint32_t* pixel = (uint32_t*)row_addr;
        for (size_t x = 0; x < fb_info.width; ++x) {
            double x0 = x * width / fb_info.width + left;
            *pixel++ = calc_pixel_value(x0, y0);
        }

        row_addr += fb_info.pitch;
    }

    (void)getchar();

    return EXIT_SUCCESS;
}
