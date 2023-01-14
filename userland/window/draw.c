#include <string.h>
#include <fb.h>

#include "draw.h"

void fill_screen(void * framebuffer, struct fb_info framebuffer_info, int color)
{
    memset(framebuffer, color, framebuffer_info.width * framebuffer_info.height * (framebuffer_info.bpp / 8));
}