#include <string.h>
#include <fb.h>

#include "draw.h"

void draw_pixel(struct draw_info object, struct fb_info framebuffer_info, void * framebuffer)
{
    ((char*)framebuffer)[object.x * (framebuffer_info.bpp / 8) + object.y * framebuffer_info.pitch] = object.r;
    ((char*)framebuffer)[object.x * (framebuffer_info.bpp / 8) + object.y * framebuffer_info.pitch + 1] = object.g;
    ((char*)framebuffer)[object.x * (framebuffer_info.bpp / 8) + object.y * framebuffer_info.pitch + 2] = object.b;
}