#pragma once

struct draw_info {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    int x;
    int y;
};

void draw_pixel(struct draw_info object, struct fb_info framebuffer_info, void * framebuffer);