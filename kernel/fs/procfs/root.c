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
#include <common/stdio.h>
#include <common/stdlib.h>
#include <kernel/api/dirent.h>
#include <kernel/fs/dentry.h>
#include <kernel/growable_buf.h>
#include <kernel/interrupts.h>
#include <kernel/panic.h>
#include <kernel/process.h>

static int populate_cmdline(file_description* desc, growable_buf* buf) {
    (void)desc;
    return growable_buf_printf(buf, "%s\n", cmdline_get_raw());
}

static int populate_meminfo(file_description* desc, growable_buf* buf) {
    (void)desc;
    struct physical_memory_info memory_info;
    page_allocator_get_info(&memory_info);

    return growable_buf_printf(buf,
                               "MemTotal: %8u kB\n"
                               "MemFree:  %8u kB\n",
                               memory_info.total, memory_info.free);
}

static int populate_uptime(file_description* desc, growable_buf* buf) {
    (void)desc;
    return growable_buf_printf(buf, "%u\n", uptime / CLK_TCK);
}
static procfs_item_def root_items[] = {{"cmdline", populate_cmdline},
                                       {"meminfo", populate_meminfo},
                                       {"uptime", populate_uptime}};
#define NUM_ITEMS (sizeof(root_items) / sizeof(procfs_item_def))

static struct inode* procfs_root_lookup_child(struct inode* inode,
                                              const char* name) {
    if (str_is_uint(name)) {
        pid_t pid = atoi(name);
        return procfs_pid_dir_inode_create((procfs_dir_inode*)inode, pid);
    }
    return procfs_dir_lookup_child(inode, name);
}

static int procfs_root_getdents(struct getdents_ctx* ctx,
                                file_description* desc,
                                getdents_callback_fn callback) {
    procfs_dir_inode* node = (procfs_dir_inode*)desc->inode;

    mutex_lock(&desc->offset_lock);
    if ((size_t)desc->offset < NUM_ITEMS) {
        int rc = dentry_getdents(ctx, desc, node->children, callback);
        if (IS_ERR(rc)) {
            mutex_unlock(&desc->offset_lock);
            return rc;
        }
    }
    if ((size_t)desc->offset < NUM_ITEMS) {
        mutex_unlock(&desc->offset_lock);
        return 0;
    }

    bool int_flag = push_cli();

    pid_t offset_pid = (pid_t)(desc->offset - NUM_ITEMS);
    struct process* it = all_processes;
    while (it->pid <= offset_pid) {
        it = it->next_in_all_processes;
        if (!it)
            break;
    }

    while (it) {
        char name[16];
        (void)snprintf(name, sizeof(name), "%d", it->pid);
        if (!callback(ctx, name, DT_DIR))
            break;
        desc->offset = it->pid + NUM_ITEMS;
        it = it->next_in_all_processes;
    }

    pop_cli(int_flag);
    mutex_unlock(&desc->offset_lock);
    return 0;
}

static int add_item(procfs_dir_inode* parent, const procfs_item_def* item_def) {
    procfs_item_inode* node = kmalloc(sizeof(procfs_item_inode));
    if (!node)
        return -ENOMEM;
    *node = (procfs_item_inode){0};

    node->populate = item_def->populate;

    struct inode* inode = &node->inode;
    inode->fs_root_inode = parent->inode.fs_root_inode;
    inode->fops = &procfs_item_fops;
    inode->mode = S_IFREG;
    inode->ref_count = 1;

    return dentry_append(&parent->children, item_def->name, inode);
}

struct inode* procfs_create_root(void) {
    procfs_dir_inode* root = kmalloc(sizeof(procfs_dir_inode));
    if (!root)
        return ERR_PTR(-ENOMEM);
    *root = (procfs_dir_inode){0};

    static file_ops fops = {
        .destroy_inode = procfs_dir_destroy_inode,
        .lookup_child = procfs_root_lookup_child,
        .getdents = procfs_root_getdents,
    };

    struct inode* inode = &root->inode;
    inode->fs_root_inode = inode;
    inode->fops = &fops;
    inode->mode = S_IFDIR;
    inode->ref_count = 1;

    for (size_t i = 0; i < NUM_ITEMS; ++i) {
        int rc = add_item(root, root_items + i);
        if (IS_ERR(rc))
            return ERR_PTR(rc);
    }

    return inode;
}
