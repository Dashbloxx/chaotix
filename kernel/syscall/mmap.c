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

#include <common/extra.h>
#include <common/string.h>
#include <kernel/api/err.h>
#include <kernel/api/sys/mman.h>
#include <kernel/api/sys/stat.h>
#include <kernel/api/sys/syscall.h>
#include <kernel/boot_defs.h>
#include <kernel/memory/memory.h>
#include <kernel/process.h>

void* sys_mmap(const mmap_params* params) {
    if (params->length == 0 || params->offset < 0 || (params->offset % PAGE_SIZE) || !((params->flags & MAP_PRIVATE) ^ (params->flags & MAP_SHARED)))
        return ERR_PTR(-EINVAL);

    if ((params->flags & MAP_FIXED) || !(params->prot & PROT_READ))
        return ERR_PTR(-ENOTSUP);

    uintptr_t addr = range_allocator_alloc(&current->vaddr_allocator, params->length);
    if (IS_ERR(addr))
        return ERR_PTR(addr);

    uint16_t page_flags = PAGE_USER;
    if (params->prot & PROT_WRITE)
        page_flags |= PAGE_WRITE;
    if (params->flags & MAP_SHARED)
        page_flags |= PAGE_SHARED;

    if (params->flags & MAP_ANONYMOUS) {
        if (params->offset != 0)
            return ERR_PTR(-ENOTSUP);

        int rc = paging_map_to_free_pages(addr, params->length, page_flags);
        if (IS_ERR(rc))
            return ERR_PTR(rc);

        memset((void*)addr, 0, params->length);
        return (void*)addr;
    }

    file_description* desc = process_get_file_description(params->fd);
    if (IS_ERR(desc))
        return desc;
    if (S_ISDIR(desc->inode->mode))
        return ERR_PTR(-ENODEV);

    return (void*)file_description_mmap(desc, addr, params->length, params->offset, page_flags);
}

int sys_munmap(void* addr, size_t length) {
    if ((uintptr_t)addr % PAGE_SIZE)
        return -EINVAL;
    paging_unmap((uintptr_t)addr, length);
    return range_allocator_free(&current->vaddr_allocator, (uintptr_t)addr, length);
}
