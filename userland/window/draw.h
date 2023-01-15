#pragma once

/*
 *  The values `width` and `height` don't apply for drawing pixels...
 */
struct pixel_info {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned int x;
    unsigned int y;
};

struct rectangle_info {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned int x;
    unsigned int y;
    unsigned int width;
    unsigned int height;
};

void draw_pixel(struct pixel_info object, struct fb_info framebuffer_info, void * framebuffer);
void draw_rectangle(struct rectangle_info object, struct fb_info framebuffer_info, void * framebuffer);