#include "graphics.h"
#include <stddef.h>

bool bochs_fb_init(void);
struct inode* bochs_fb_device_create(void);

bool multiboot_fb_init(const multiboot_info_t* mb_info);
struct inode* multiboot_fb_device_create(void);

static bool bochs_is_available = false;
static bool multiboot_is_available = false;

bool fb_init(const multiboot_info_t* mb_info) {
    if (bochs_fb_init()) {
        bochs_is_available = true;
        return true;
    }
    if (multiboot_fb_init(mb_info)) {
        multiboot_is_available = true;
        return true;
    }
    return false;
}

struct inode* fb_device_create(void) {
    if (bochs_is_available)
        return bochs_fb_device_create();
    if (multiboot_is_available)
        return multiboot_fb_device_create();
    return NULL;
}
