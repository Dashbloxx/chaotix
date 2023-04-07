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

#include "random.h"
#include "api/err.h"
#include "api/sys/sysmacros.h"
#include "fs/fs.h"
#include "memory/memory.h"
#include "system.h"
#include <common/string.h>

/*
 *  The reason why the random character device is created here, is because theoretically there can be actual hardware for generating
 *  random numbers that don't depend on other factors (HRNGs which get randomness by observing radioactive decay). 
 */

static int rand_state = 1;

int rand() {
    rand_state = ((rand_state * 1103515245U) + 12345U) & 0x7fffffff;
    return rand_state;
}

unsigned int urand() {
    rand_state = ((rand_state * 1103515245U) + 12345U) & 0xffffffffU;
    return rand_state;
}

void srand(unsigned seed) { rand_state = seed == 0 ? 1 : seed; }

static int write_to_bit_bucket(file_description* desc, const void* buffer, unsigned int count) {
    (void)desc;
    (void)buffer;
    return count;
}

static int read_random(file_description* desc, void* buffer, unsigned int count) {
    (void)desc;
    unsigned int* uint_buffer = (unsigned int*)buffer;
    for (unsigned int i = 0; i < count / sizeof(unsigned int); i++) {
        uint_buffer[i] = urand();
    }
    return count;
}

struct inode* random_device_create() {
    struct inode* inode = kmalloc(sizeof(struct inode));
    if (!inode)
        return ERR_PTR(-ENOMEM);

    static file_ops fops = {.read = read_random, .write = write_to_bit_bucket};
    *inode = (struct inode){.fops = &fops,
                            .mode = S_IFCHR,
                            .device_id = makedev(1, 9),
                            .ref_count = 1};
    return inode;
}