#include <errno.h>
#include <extra.h>
#include <fb.h>
#include <fcntl.h>
#include <hid.h>
#include <panic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#define CURSOR_WIDTH 8
#define CURSOR_HEIGHT 14
#define CURSOR_COLOR 0xf0f0f0

static const char mask[] = "x......."
                           "xx......"
                           "xxx....."
                           "xxxx...."
                           "xxxxx..."
                           "xxxxxx.."
                           "xxxxxxx."
                           "xxxxxxxx"
                           "xxxxxxx."
                           "xxxxx..."
                           "x...xx.."
                           "....xx.."
                           ".....xx."
                           ".....xx.";

static uintptr_t fb_addr;
static struct fb_info fb_info;
static int32_t cursor_x;
static int32_t cursor_y;
static size_t visible_width;
static size_t visible_height;
static uint32_t fb_buf[CURSOR_WIDTH * CURSOR_HEIGHT];

static void move_cursor_to(int32_t x, int32_t y) {
    cursor_x = MAX(0, MIN((int32_t)(fb_info.width - 1), x));
    cursor_y = MAX(0, MIN((int32_t)(fb_info.height - 1), y));
    visible_width = MIN(CURSOR_WIDTH, fb_info.width - cursor_x);
    visible_height = MIN(CURSOR_HEIGHT, fb_info.height - cursor_y);

    uintptr_t origin_addr =
        fb_addr + cursor_x * sizeof(uint32_t) + cursor_y * fb_info.pitch;

    // save framebuffer content to fb_buf
    uintptr_t row_addr = origin_addr;
    uint32_t* dest = fb_buf;
    for (size_t y = 0; y < visible_height; ++y) {
        memcpy32(dest, (uint32_t*)row_addr, visible_width);
        row_addr += fb_info.pitch;
        dest += visible_width;
    }

    // draw cursor
    row_addr = origin_addr;
    char* row_mask = (char*)mask;
    for (size_t y = 0; y < visible_height; ++y) {
        uint32_t* pixel = (uint32_t*)row_addr;
        char* mask_for_pixel = row_mask;
        for (size_t x = 0; x < visible_width; ++x) {
            if (*mask_for_pixel++ == 'x')
                *pixel = CURSOR_COLOR;
            ++pixel;
        }
        row_addr += fb_info.pitch;
        row_mask += CURSOR_WIDTH;
    }
}

static void restore_fb(void) {
    uintptr_t row_addr =
        fb_addr + cursor_x * sizeof(uint32_t) + cursor_y * fb_info.pitch;
    uint32_t* src = fb_buf;
    for (size_t y = 0; y < visible_height; ++y) {
        memcpy32((uint32_t*)row_addr, src, visible_width);
        row_addr += fb_info.pitch;
        src += visible_width;
    }
}

int main(void) {
    int fb_fd = open("/dev/fb0", O_RDWR);
    if (fb_fd < 0) {
        if (errno == ENOENT)
            return EXIT_SUCCESS;
        perror("open");
        return EXIT_FAILURE;
    }
    if (ioctl(fb_fd, FBIOGET_INFO, &fb_info) < 0) {
        perror("ioctl");
        close(fb_fd);
        return EXIT_FAILURE;
    }
    ASSERT(fb_info.bpp == 32);
    void* fb = mmap(NULL, fb_info.pitch * fb_info.height,
                    PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, 0);
    close(fb_fd);
    if (fb == MAP_FAILED) {
        perror("mmap");
        return EXIT_FAILURE;
    }
    fb_addr = (uintptr_t)fb;

    move_cursor_to(fb_info.width / 2, fb_info.height / 2);

    int mouse_fd = open("/dev/psaux", O_RDONLY);
    if (mouse_fd < 0) {
        perror("open");
        return EXIT_FAILURE;
    }

    mouse_event event;
    for (;;) {
        ssize_t nread = read(mouse_fd, &event, sizeof(mouse_event));
        if (nread < 0) {
            perror("read");
            return EXIT_FAILURE;
        }
        restore_fb();
        move_cursor_to(cursor_x + event.dx, cursor_y + event.dy);
    }

    close(mouse_fd);
    return EXIT_SUCCESS;
}
