/*
 *  .OOOOOO.   OOOO                                .    O8O              
 *  D8P'  `Y8B  `888                              .O8    `"'              
 * 888           888 .OO.    .OOOO.    .OOOOO.  .O888OO OOOO  OOOO    OOO 
 * 888           888P"Y88B  `P  )88B  D88' `88B   888   `888   `88B..8P'  
 * 888           888   888   .OP"888  888   888   888    888     Y888'    
 * `88B    OOO   888   888  D8(  888  888   888   888 .  888   .O8"'88B   
 *  `Y8BOOD8P'  O888O O888O `Y888""8O `Y8BOD8P'   "888" O888O O88'   888O 
 * 
 *  Chaotix is a UNIX-like operating system that consists of a kernel written in C and
 *  i?86 assembly, and userland binaries written in C.
 *     
 *  Copyright (c) 2023 Nexuss
 *  Copyright (c) 2022 mosm
 *  Copyright (c) 2006-2018 Frans Kaashoek, Robert Morris, Russ Cox, Massachusetts Institute of Technology
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

#include "fs.h"
#include <common/extra.h>
#include <common/string.h>
#include <kernel/api/fcntl.h>
#include <kernel/boot_defs.h>
#include <kernel/memory/memory.h>
#include <kernel/panic.h>

struct cpio_odc_header {
    char c_magic[6];
    char c_dev[6];
    char c_ino[6];
    char c_mode[6];
    char c_uid[6];
    char c_gid[6];
    char c_nlink[6];
    char c_rdev[6];
    char c_mtime[11];
    char c_namesize[6];
    char c_filesize[11];
} __attribute__((packed));

static size_t parse_octal(const char* s, size_t len) {
    size_t res = 0;
    for (size_t i = 0; i < len; ++i) {
        res += s[i] - '0';
        if (i < len - 1)
            res *= 8;
    }
    return res;
}

#define PARSE(field) parse_octal(field, sizeof(field))

void initrd_populate_root_fs(uintptr_t paddr, size_t size) {
    uintptr_t region_start = round_down(paddr, PAGE_SIZE);
    uintptr_t region_end = paddr + size;
    size_t region_size = region_end - region_start;

    uintptr_t vaddr =
        range_allocator_alloc(&kernel_vaddr_allocator, region_size);
    ASSERT_OK(vaddr);
    ASSERT_OK(
        paging_map_to_physical_range(vaddr, region_start, region_size, 0));

    uintptr_t cursor = vaddr + (paddr - region_start);
    for (;;) {
        const struct cpio_odc_header* header =
            (const struct cpio_odc_header*)cursor;
        ASSERT(!strncmp(header->c_magic, "070707", 6));

        size_t name_size = PARSE(header->c_namesize);
        const char* filename =
            (const char*)(cursor + sizeof(struct cpio_odc_header));
        if (!strncmp(filename, "TRAILER!!!", name_size))
            break;

        size_t mode = PARSE(header->c_mode);
        size_t file_size = PARSE(header->c_filesize);

        if (S_ISDIR(mode)) {
            ASSERT_OK(vfs_create(filename, mode));
        } else {
            file_description* desc =
                vfs_open(filename, O_CREAT | O_EXCL | O_WRONLY, mode);
            ASSERT_OK(desc);

            desc->inode->device_id = PARSE(header->c_rdev);

            const unsigned char* file_content =
                (const unsigned char*)(cursor + sizeof(struct cpio_odc_header) +
                                       name_size);
            for (size_t count = 0; count < file_size;) {
                ssize_t nwritten = file_description_write(
                    desc, file_content + count, file_size - count);
                ASSERT_OK(nwritten);
                count += nwritten;
            }

            ASSERT_OK(file_description_close(desc));
        }

        cursor += sizeof(struct cpio_odc_header) + name_size + file_size;
    }

    paging_unmap(vaddr, region_size);
    ASSERT_OK(
        range_allocator_free(&kernel_vaddr_allocator, vaddr, region_size));
}
