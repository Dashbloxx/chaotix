#pragma once

#include <stddef.h>

enum { FBIOGET_INFO, FBIOSET_INFO };

struct fb_info {
    size_t width;
    size_t height;
    size_t pitch;
    size_t bpp;
};
