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

#include "boot_defs.h"
#include "kprintf.h"
#include "multiboot.h"
#include "system.h"
#include <common/string.h>
//#include <string.h>

#define MAX_CMDLINE_LEN 1024
#define MAX_NUM_KEYS 1024

static char raw[MAX_CMDLINE_LEN];
static char buf[MAX_CMDLINE_LEN];
static size_t num_keys = 0;
static char* keys[MAX_NUM_KEYS];
static char* values[MAX_NUM_KEYS];

void cmdline_init(const multiboot_info_t* mb_info) {
    if (!(mb_info->flags & MULTIBOOT_INFO_CMDLINE))
        return;

    const char* str = (const char*)(mb_info->cmdline + KERNEL_VADDR);
    kprintf("Kernel cmdline: \"%s\"\n", str);
    strlcpy(raw, str, MAX_CMDLINE_LEN);
    strlcpy(buf, str, MAX_CMDLINE_LEN);

    char* saved_ptr;
    static const char* sep = " ";
    for (char* token = strtok_r(buf, sep, &saved_ptr); token;
         token = strtok_r(NULL, sep, &saved_ptr)) {
        keys[num_keys] = token;

        char* next_equal = strchr(token, '=');
        char* next_space = strchr(token, ' ');
        if (next_equal) {
            if (!next_space || next_equal < next_space)
                values[num_keys] = next_equal + 1;
        }

        ++num_keys;
    }

    // null terminate
    for (size_t i = 0; i < num_keys; ++i) {
        char* space = strchr(keys[i], ' ');
        if (space)
            *space = 0;
        if (values[i]) {
            char* equal = strchr(keys[i], '=');
            if (equal)
                *equal = 0;
        }
    }
}

const char* cmdline_get_raw(void) { return raw; }

const char* cmdline_lookup(const char* key) {
    if (num_keys == 0)
        return NULL;
    for (size_t i = 0; i < num_keys; ++i) {
        if (!strcmp(keys[i], key))
            return values[i];
    }
    return NULL;
}

bool cmdline_contains(const char* key) {
    if (num_keys == 0)
        return false;
    for (size_t i = 0; i < num_keys; ++i) {
        if (!strcmp(keys[i], key))
            return true;
    }
    return false;
}
