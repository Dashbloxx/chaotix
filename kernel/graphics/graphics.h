#pragma once

#include <kernel/forward.h>
#include <stdbool.h>

bool fb_init(const multiboot_info_t* mb_info);
struct inode* fb_device_create(void);
