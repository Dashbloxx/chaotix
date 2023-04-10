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

#include "procfs.h"
#include <kernel/fs/dentry.h>
#include <kernel/growable_buf.h>
#include <kernel/memory/memory.h>
#include <kernel/panic.h>

static void procfs_item_destroy_inode(struct inode* inode) { kfree(inode); }

static int procfs_item_open(file_description* desc, int flags, mode_t mode) {
    (void)flags;
    (void)mode;

    growable_buf* buf = kmalloc(sizeof(growable_buf));
    if (!buf)
        return -ENOMEM;
    *buf = (growable_buf){0};

    procfs_item_inode* node = (procfs_item_inode*)desc->inode;
    int rc = node->populate(desc, buf);
    if (IS_ERR(rc)) {
        kfree(buf);
        return rc;
    }

    desc->private_data = buf;
    return 0;
}

static int procfs_item_close(file_description* desc) {
    growable_buf_destroy(desc->private_data);
    kfree(desc->private_data);
    return 0;
}

static ssize_t procfs_item_read(file_description* desc, void* buffer,
                                size_t count) {
    mutex_lock(&desc->offset_lock);
    ssize_t nread =
        growable_buf_pread(desc->private_data, buffer, count, desc->offset);
    if (IS_OK(nread))
        desc->offset += nread;
    mutex_unlock(&desc->offset_lock);
    return nread;
}

file_ops procfs_item_fops = {.destroy_inode = procfs_item_destroy_inode,
                             .open = procfs_item_open,
                             .close = procfs_item_close,
                             .read = procfs_item_read};

void procfs_dir_destroy_inode(struct inode* inode) {
    procfs_dir_inode* node = (procfs_dir_inode*)inode;
    dentry_clear(node->children);
    kfree(node);
}

struct inode* procfs_dir_lookup_child(struct inode* inode, const char* name) {
    procfs_dir_inode* node = (procfs_dir_inode*)inode;
    struct inode* child = dentry_find(node->children, name);
    inode_unref(inode);
    return child;
}

int procfs_dir_getdents(struct getdents_ctx* ctx, file_description* desc,
                        getdents_callback_fn callback) {
    procfs_dir_inode* node = (procfs_dir_inode*)desc->inode;
    mutex_lock(&desc->offset_lock);
    int rc = dentry_getdents(ctx, desc, node->children, callback);
    mutex_unlock(&desc->offset_lock);
    return rc;
}
