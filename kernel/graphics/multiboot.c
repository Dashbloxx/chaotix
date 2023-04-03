/*
 *    __  __                      
 *   |  \/  |__ _ __ _ _ __  __ _ 
 *   | |\/| / _` / _` | '  \/ _` |
 *   |_|  |_\__,_\__, |_|_|_\__,_|
 *               |___/        
 * 
 *  Magma is a UNIX-like operating system that consists of a kernel written in C and
 *  i?86 assembly, and userland binaries written in C.
 *     
 *  Copyright (c) 2023 Nexuss, John Paul Wohlscheid, rilysh, Milton612, and FueledByCocaine
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

#include <kernel/api/err.h>
#include <kernel/api/fb.h>
#include <kernel/api/sys/sysmacros.h>
#include <kernel/fs/fs.h>
#include <kernel/kprintf.h>
#include <kernel/memory/memory.h>
#include <kernel/multiboot.h>

static uintptr_t fb_paddr;
static struct fb_info fb_info;

bool multiboot_fb_init(const multiboot_info_t* mb_info) {
    if (!(mb_info->flags & MULTIBOOT_INFO_FRAMEBUFFER_INFO))
        return false;
    if (mb_info->framebuffer_type != MULTIBOOT_FRAMEBUFFER_TYPE_RGB)
        return false;

    fb_paddr = mb_info->framebuffer_addr;
    fb_info.width = mb_info->framebuffer_width;
    fb_info.height = mb_info->framebuffer_height;
    fb_info.pitch = mb_info->framebuffer_pitch;
    fb_info.bpp = mb_info->framebuffer_bpp;
    kprintf("Found framebuffer at P0x%x\n", fb_paddr);
    return true;
}

static uintptr_t multiboot_fb_device_mmap(file_description* desc,
                                          uintptr_t addr, size_t length,
                                          off_t offset, uint16_t page_flags) {
    (void)desc;
    if (offset != 0)
        return -ENXIO;
    if (!(page_flags & PAGE_SHARED))
        return -ENODEV;

    int rc = paging_map_to_physical_range(addr, fb_paddr, length,
                                          page_flags | PAGE_PAT);
    if (IS_ERR(rc))
        return rc;
    return addr;
}

static int multiboot_fb_device_ioctl(file_description* desc, int request,
                                     void* argp) {
    (void)desc;
    switch (request) {
    case FBIOGET_INFO:
        *(struct fb_info*)argp = fb_info;
        return 0;
    case FBIOSET_INFO:
        return -ENOTSUP;
    }
    return -EINVAL;
}

struct inode* multiboot_fb_device_create(void) {
    struct inode* inode = kmalloc(sizeof(struct inode));
    if (!inode)
        return ERR_PTR(-ENOMEM);

    static file_ops fops = {.mmap = multiboot_fb_device_mmap,
                            .ioctl = multiboot_fb_device_ioctl};
    *inode = (struct inode){.fops = &fops,
                            .mode = S_IFBLK,
                            .device_id = makedev(29, 0),
                            .ref_count = 1};
    return inode;
}
