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
#include <kernel/growable_buf.h>
#include <kernel/memory/memory.h>
#include <kernel/panic.h>

typedef struct tmpfs_inode {
    struct inode inode;
    growable_buf buf;
    mutex children_lock;
    struct dentry* children;
} tmpfs_inode;

static void tmpfs_destroy_inode(struct inode* inode) {
    tmpfs_inode* node = (tmpfs_inode*)inode;
    growable_buf_destroy(&node->buf);
    dentry_clear(node->children);
    kfree(node);
}

static struct inode* tmpfs_lookup_child(struct inode* inode, const char* name) {
    tmpfs_inode* node = (tmpfs_inode*)inode;
    mutex_lock(&node->children_lock);
    struct inode* child = dentry_find(node->children, name);
    mutex_unlock(&node->children_lock);
    inode_unref(inode);
    return child;
}

static int tmpfs_stat(struct inode* inode, struct stat* buf) {
    tmpfs_inode* node = (tmpfs_inode*)inode;
    buf->st_mode = inode->mode;
    buf->st_nlink = inode->num_links;
    buf->st_rdev = 0;
    buf->st_size = node->buf.size;
    inode_unref(inode);
    return 0;
}

static ssize_t tmpfs_read(file_description* desc, void* buffer, size_t count) {
    tmpfs_inode* node = (tmpfs_inode*)desc->inode;
    mutex_lock(&desc->offset_lock);
    mutex_lock(&node->buf.lock);
    ssize_t nread = growable_buf_pread(&node->buf, buffer, count, desc->offset);
    mutex_unlock(&node->buf.lock);
    if (IS_OK(nread))
        desc->offset += nread;
    mutex_unlock(&desc->offset_lock);
    return nread;
}

static ssize_t tmpfs_write(file_description* desc, const void* buffer,
                           size_t count) {
    tmpfs_inode* node = (tmpfs_inode*)desc->inode;
    mutex_lock(&desc->offset_lock);
    mutex_lock(&node->buf.lock);
    ssize_t nwritten =
        growable_buf_pwrite(&node->buf, buffer, count, desc->offset);
    mutex_unlock(&node->buf.lock);
    if (IS_OK(nwritten))
        desc->offset += nwritten;
    mutex_unlock(&desc->offset_lock);
    return nwritten;
}

static uintptr_t tmpfs_mmap(file_description* desc, uintptr_t addr,
                            size_t length, off_t offset, uint16_t page_flags) {
    tmpfs_inode* node = (tmpfs_inode*)desc->inode;
    mutex_lock(&node->buf.lock);
    uintptr_t rc =
        growable_buf_mmap(&node->buf, addr, length, offset, page_flags);
    mutex_unlock(&node->buf.lock);
    return rc;
}

static int tmpfs_truncate(file_description* desc, off_t length) {
    tmpfs_inode* node = (tmpfs_inode*)desc->inode;
    mutex_lock(&node->buf.lock);
    int rc = growable_buf_truncate(&node->buf, length);
    mutex_unlock(&node->buf.lock);
    return rc;
}

static int tmpfs_getdents(struct getdents_ctx* ctx, file_description* desc,
                          getdents_callback_fn callback) {
    tmpfs_inode* node = (tmpfs_inode*)desc->inode;
    mutex_lock(&node->children_lock);
    mutex_lock(&desc->offset_lock);
    int rc = dentry_getdents(ctx, desc, node->children, callback);
    mutex_unlock(&desc->offset_lock);
    mutex_unlock(&node->children_lock);
    return rc;
}

static int tmpfs_link_child(struct inode* inode, const char* name,
                            struct inode* child) {
    tmpfs_inode* node = (tmpfs_inode*)inode;
    mutex_lock(&node->children_lock);
    int rc = dentry_append(&node->children, name, child);
    mutex_unlock(&node->children_lock);
    inode_unref(inode);
    return rc;
}

static struct inode* tmpfs_unlink_child(struct inode* inode, const char* name) {
    tmpfs_inode* node = (tmpfs_inode*)inode;
    mutex_lock(&node->children_lock);
    struct inode* child = dentry_remove(&node->children, name);
    mutex_unlock(&node->children_lock);
    inode_unref(inode);
    return child;
}

static struct inode* tmpfs_create_child(struct inode* inode, const char* name,
                                        mode_t mode);

static file_ops dir_fops = {.destroy_inode = tmpfs_destroy_inode,
                            .lookup_child = tmpfs_lookup_child,
                            .create_child = tmpfs_create_child,
                            .link_child = tmpfs_link_child,
                            .unlink_child = tmpfs_unlink_child,
                            .stat = tmpfs_stat,
                            .getdents = tmpfs_getdents};
static file_ops non_dir_fops = {.destroy_inode = tmpfs_destroy_inode,
                                .stat = tmpfs_stat,
                                .read = tmpfs_read,
                                .write = tmpfs_write,
                                .mmap = tmpfs_mmap,
                                .truncate = tmpfs_truncate};

static struct inode* tmpfs_create_child(struct inode* inode, const char* name,
                                        mode_t mode) {
    tmpfs_inode* child = kmalloc(sizeof(tmpfs_inode));
    if (!child)
        return ERR_PTR(-ENOMEM);
    *child = (tmpfs_inode){0};

    struct inode* child_inode = &child->inode;
    child_inode->fs_root_inode = inode->fs_root_inode;
    child_inode->fops = S_ISDIR(mode) ? &dir_fops : &non_dir_fops;
    child_inode->mode = mode;
    child_inode->ref_count = 1;

    inode_ref(child_inode);
    int rc = tmpfs_link_child(inode, name, child_inode);
    if (IS_ERR(rc)) {
        kfree(child);
        return ERR_PTR(rc);
    }

    return child_inode;
}

struct inode* tmpfs_create_root(void) {
    tmpfs_inode* root = kmalloc(sizeof(tmpfs_inode));
    if (!root)
        return ERR_PTR(-ENOMEM);
    *root = (tmpfs_inode){0};

    struct inode* inode = &root->inode;
    inode->fs_root_inode = inode;
    inode->fops = &dir_fops;
    inode->mode = S_IFDIR;
    inode->ref_count = 1;

    return inode;
}
