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

#include "dentry.h"
#include <common/string.h>
#include <kernel/api/dirent.h>
#include <kernel/api/err.h>
#include <kernel/memory/memory.h>
#include <kernel/panic.h>

struct inode* dentry_find(const struct dentry* head, const char* name) {
    const struct dentry* dentry = head;
    while (dentry) {
        if (!strcmp(dentry->name, name)) {
            inode_ref(dentry->inode);
            return dentry->inode;
        }
        dentry = dentry->next;
    }
    return ERR_PTR(-ENOENT);
}

int dentry_getdents(struct getdents_ctx* ctx, file_description* desc,
                    const struct dentry* head, getdents_callback_fn callback) {
    const struct dentry* dentry = head;
    if (!dentry)
        return 0;

    for (off_t i = 0; i < desc->offset; ++i) {
        dentry = dentry->next;
        if (!dentry)
            return 0;
    }

    for (; dentry; dentry = dentry->next) {
        uint8_t type = mode_to_dirent_type(dentry->inode->mode);
        if (!callback(ctx, dentry->name, type))
            break;
        ++desc->offset;
    }
    return 0;
}

int dentry_append(struct dentry** head, const char* name, struct inode* child) {
    struct dentry* prev = NULL;
    struct dentry* it = *head;
    while (it) {
        if (!strcmp(it->name, name))
            return -EEXIST;
        prev = it;
        it = it->next;
    }

    struct dentry* new_dentry = kmalloc(sizeof(struct dentry));
    if (!new_dentry)
        return -ENOMEM;
    *new_dentry = (struct dentry){0};

    new_dentry->name = kstrdup(name);
    if (!new_dentry->name) {
        kfree(new_dentry);
        return -ENOMEM;
    }
    new_dentry->inode = child;

    if (prev)
        prev->next = new_dentry;
    else
        *head = new_dentry;

    ++child->num_links;

    return 0;
}

struct inode* dentry_remove(struct dentry** head, const char* name) {
    struct dentry* prev = NULL;
    struct dentry* it = *head;
    while (it) {
        if (!strcmp(it->name, name)) {
            if (prev)
                prev->next = it->next;
            else
                *head = it->next;
            struct inode* inode = it->inode;
            kfree(it->name);
            kfree(it);
            ASSERT(inode->num_links > 0);
            --inode->num_links;
            return inode;
        }
        prev = it;
        it = it->next;
    }
    return ERR_PTR(-ENOENT);
}

void dentry_clear(struct dentry* head) {
    for (struct dentry* dentry = head; dentry;) {
        struct dentry* next = dentry->next;
        ASSERT(dentry->inode->num_links > 0);
        --dentry->inode->num_links;
        inode_unref(dentry->inode);
        kfree(dentry->name);
        kfree(dentry);
        dentry = next;
    }
}
