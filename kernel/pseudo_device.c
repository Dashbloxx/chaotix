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

#include "api/err.h"
#include "api/sys/sysmacros.h"
#include "fs/fs.h"
#include "memory/memory.h"
#include "system.h"
#include <common/string.h>
//#include <string.h>

static ssize_t read_nothing(file_description* desc, void* buffer,
                            size_t count) {
    (void)desc;
    (void)buffer;
    (void)count;
    return 0;
}

static ssize_t read_zeros(file_description* desc, void* buffer, size_t count) {
    (void)desc;
    memset(buffer, 0, count);
    return count;
}

static ssize_t write_to_bit_bucket(file_description* desc, const void* buffer, size_t count) {
    (void)desc;
    (void)buffer;
    return count;
}

static ssize_t write_to_full_disk(file_description* desc, const void* buffer, size_t count) {
    (void)desc;
    (void)buffer;
    if (count > 0)
        return -ENOSPC;
    return 0;
}

struct inode* null_device_create(void) {
    struct inode* inode = kmalloc(sizeof(struct inode));
    if (!inode)
        return ERR_PTR(-ENOMEM);

    static file_ops fops = {.read = read_nothing, .write = write_to_bit_bucket};
    *inode = (struct inode){.fops = &fops,
                            .mode = S_IFCHR,
                            .device_id = makedev(1, 3),
                            .ref_count = 1};
    return inode;
}

struct inode* zero_device_create(void) {
    struct inode* inode = kmalloc(sizeof(struct inode));
    if (!inode)
        return ERR_PTR(-ENOMEM);

    static file_ops fops = {.read = read_zeros, .write = write_to_bit_bucket};
    *inode = (struct inode){.fops = &fops,
                            .mode = S_IFCHR,
                            .device_id = makedev(1, 5),
                            .ref_count = 1};
    return inode;
}

struct inode* full_device_create(void) {
    struct inode* inode = kmalloc(sizeof(struct inode));
    if (!inode)
        return ERR_PTR(-ENOMEM);

    static file_ops fops = {.read = read_zeros, .write = write_to_full_disk};
    *inode = (struct inode){.fops = &fops,
                            .mode = S_IFCHR,
                            .device_id = makedev(1, 7),
                            .ref_count = 1};
    return inode;
}
