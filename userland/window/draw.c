#include <string.h>
#include <fb.h>

#include "draw.h"

void draw_pixel(struct pixel_info object, struct fb_info framebuffer_info, void * framebuffer)
{
    ((char*)framebuffer)[object.x * (framebuffer_info.bpp / 8) + object.y * framebuffer_info.pitch] = object.r;
    ((char*)framebuffer)[object.x * (framebuffer_info.bpp / 8) + object.y * framebuffer_info.pitch + 1] = object.g;
    ((char*)framebuffer)[object.x * (framebuffer_info.bpp / 8) + object.y * framebuffer_info.pitch + 2] = object.b;
}

void draw_rectangle(struct rectangle_info object, struct fb_info framebuffer_info, void * framebuffer)
{
    for(unsigned int x = object.x; x < object.width; x++)
    {
        for(unsigned int y = object.y; y < object.height; y++)
        {
            ((char*)framebuffer)[x * (framebuffer_info.bpp / 8) + y * framebuffer_info.pitch] = object.r;
            ((char*)framebuffer)[x * (framebuffer_info.bpp / 8) + y * framebuffer_info.pitch + 1] = object.g;
            ((char*)framebuffer)[x * (framebuffer_info.bpp / 8) + y * framebuffer_info.pitch + 2] = object.b;
        }
    }
}