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

#include "fs.h"
#include <common/string.h>
#include <kernel/api/dirent.h>
#include <kernel/api/fcntl.h>
#include <kernel/api/stdio.h>
#include <kernel/lock.h>
#include <kernel/memory/memory.h>
#include <kernel/panic.h>
#include <kernel/scheduler.h>

int file_descriptor_table_init(file_descriptor_table* table) {
    table->entries = kmalloc(OPEN_MAX * sizeof(file_description*));
    if (!table->entries)
        return -ENOMEM;

    for (size_t i = 0; i < OPEN_MAX; ++i)
        table->entries[i] = NULL;
    return 0;
}

void file_descriptor_table_destroy(file_descriptor_table* table) {
    file_description** it = table->entries;
    for (int i = 0; i < OPEN_MAX; ++i, ++it) {
        if (*it)
            file_description_close(*it);
    }
    kfree(table->entries);
}

int file_descriptor_table_clone_from(file_descriptor_table* to,
                                     const file_descriptor_table* from) {
    to->entries = kmalloc(OPEN_MAX * sizeof(file_description*));
    if (!to->entries)
        return -ENOMEM;

    memcpy(to->entries, from->entries, OPEN_MAX * sizeof(file_description*));

    for (size_t i = 0; i < OPEN_MAX; ++i) {
        if (from->entries[i])
            ++from->entries[i]->ref_count;
    }
    return 0;
}

void inode_ref(struct inode* inode) {
    ASSERT(inode);
    ++inode->ref_count;
}

void inode_unref(struct inode* inode) {
    if (!inode)
        return;
    ASSERT(inode->ref_count > 0);
    if (--inode->ref_count == 0 && inode->num_links == 0)
        inode_destroy(inode);
}

void inode_destroy(struct inode* inode) {
    ASSERT(inode->ref_count == 0 && inode->num_links == 0);
    ASSERT(inode->fops->destroy_inode);
    inode->fops->destroy_inode(inode);
}

struct inode* inode_lookup_child(struct inode* inode, const char* name) {
    if (!inode->fops->lookup_child || !S_ISDIR(inode->mode)) {
        inode_unref(inode);
        return ERR_PTR(-ENOTDIR);
    }
    return inode->fops->lookup_child(inode, name);
}

struct inode* inode_create_child(struct inode* inode, const char* name, mode_t mode) {
    if (!inode->fops->create_child || !S_ISDIR(inode->mode)) {
        inode_unref(inode);
        return ERR_PTR(-ENOTDIR);
    }
    ASSERT(mode & S_IFMT);
    struct inode* child = inode->fops->create_child(inode, name, mode);
    if (IS_ERR(child))
        return child;
    return child;
}

int inode_link_child(struct inode* inode, const char* name,
                     struct inode* child) {
    if (!inode->fops->link_child || !S_ISDIR(inode->mode)) {
        inode_unref(inode);
        inode_unref(child);
        return -ENOTDIR;
    }
    if (inode->fs_root_inode != child->fs_root_inode) {
        inode_unref(inode);
        inode_unref(child);
        return -EXDEV;
    }
    inode_ref(child);
    int rc = inode->fops->link_child(inode, name, child);
    if (IS_ERR(rc)) {
        inode_unref(child);
        return rc;
    }
    inode_unref(child);
    return 0;
}

int inode_unlink_child(struct inode* inode, const char* name) {
    if (!inode->fops->unlink_child || !S_ISDIR(inode->mode)) {
        inode_unref(inode);
        return -ENOTDIR;
    }
    struct inode* child = inode->fops->unlink_child(inode, name);
    if (IS_ERR(child))
        return PTR_ERR(child);
    inode_unref(child);
    return 0;
}

file_description* inode_open(struct inode* inode, int flags, mode_t mode) {
    if (S_ISDIR(inode->mode) && (flags & O_WRONLY)) {
        inode_unref(inode);
        return ERR_PTR(-EISDIR);
    }

    file_description* desc = kmalloc(sizeof(file_description));
    if (!desc) {
        inode_unref(inode);
        return ERR_PTR(-ENOMEM);
    }
    *desc = (file_description){0};
    desc->inode = inode;
    desc->flags = flags;
    desc->ref_count = 1;

    if (inode->fops->open) {
        int rc = inode->fops->open(desc, flags, mode);
        if (IS_ERR(rc)) {
            inode_unref(inode);
            kfree(desc);
            return ERR_PTR(rc);
        }
    }
    return desc;
}

int inode_stat(struct inode* inode, struct stat* buf) {
    if (inode->fops->stat)
        return inode->fops->stat(inode, buf);
    buf->st_rdev = inode->device_id;
    buf->st_mode = inode->mode;
    buf->st_nlink = inode->num_links;
    buf->st_size = 0;
    inode_unref(inode);
    return 0;
}

int file_description_close(file_description* desc) {
    ASSERT(desc->ref_count > 0);
    if (--desc->ref_count > 0)
        return 0;
    struct inode* inode = desc->inode;
    if (inode->fops->close) {
        int rc = inode->fops->close(desc);
        if (IS_ERR(rc))
            return rc;
    }
    kfree(desc);
    inode_unref(inode);
    return 0;
}

ssize_t file_description_read(file_description* desc, void* buffer,
                              size_t count) {
    struct inode* inode = desc->inode;
    if (S_ISDIR(inode->mode))
        return -EISDIR;
    if (!inode->fops->read)
        return -EINVAL;
    if (!(desc->flags & O_RDONLY))
        return -EBADF;
    return inode->fops->read(desc, buffer, count);
}

ssize_t file_description_write(file_description* desc, const void* buffer,
                               size_t count) {
    struct inode* inode = desc->inode;
    if (S_ISDIR(inode->mode))
        return -EISDIR;
    if (!inode->fops->write)
        return -EINVAL;
    if (!(desc->flags & O_WRONLY))
        return -EBADF;
    return inode->fops->write(desc, buffer, count);
}

uintptr_t file_description_mmap(file_description* desc, uintptr_t addr,
                                size_t length, off_t offset,
                                uint16_t page_flags) {
    struct inode* inode = desc->inode;
    if (!inode->fops->mmap)
        return -ENODEV;
    if (!(desc->flags & O_RDONLY))
        return -EACCES;
    if ((page_flags & PAGE_SHARED) && (page_flags & PAGE_WRITE) &&
        ((desc->flags & O_RDWR) != O_RDWR))
        return -EACCES;
    return inode->fops->mmap(desc, addr, length, offset, page_flags);
}

int file_description_truncate(file_description* desc, off_t length) {
    struct inode* inode = desc->inode;
    if (S_ISDIR(inode->mode))
        return -EISDIR;
    if (!inode->fops->truncate)
        return -EROFS;
    if (!(desc->flags & O_WRONLY))
        return -EBADF;
    return inode->fops->truncate(desc, length);
}

off_t file_description_lseek(file_description* desc, off_t offset, int whence) {
    off_t new_offset;
    switch (whence) {
    case SEEK_SET:
        new_offset = offset;
        break;
    case SEEK_CUR:
        mutex_lock(&desc->offset_lock);
        new_offset = desc->offset + offset;
        break;
    case SEEK_END: {
        struct stat stat;
        inode_ref(desc->inode);
        int rc = inode_stat(desc->inode, &stat);
        if (IS_ERR(rc))
            return rc;
        new_offset = stat.st_size + offset;
        break;
    }
    default:
        return -EINVAL;
    }
    if (new_offset < 0) {
        mutex_unlock_if_locked(&desc->offset_lock);
        return -EINVAL;
    }
    desc->offset = new_offset;
    mutex_unlock_if_locked(&desc->offset_lock);
    return new_offset;
}

int file_description_ioctl(file_description* desc, int request, void* argp) {
    struct inode* inode = desc->inode;
    if (!inode->fops->ioctl)
        return -ENOTTY;
    return inode->fops->ioctl(desc, request, argp);
}

struct getdents_ctx {
    unsigned char* dirp;
    unsigned remaining_count;
    long nwritten;
    bool buffer_is_too_small;
};

static bool getdents_callback(struct getdents_ctx* ctx, const char* name,
                              uint8_t type) {
    size_t name_len = strlen(name);
    size_t name_size = name_len + 1;
    size_t size = offsetof(struct dirent, d_name) + name_size;
    if (ctx->remaining_count < size) {
        ctx->buffer_is_too_small = true;
        return false;
    }

    struct dirent* dent = (struct dirent*)ctx->dirp;
    dent->d_type = type;
    dent->d_reclen = size;
    dent->d_namlen = name_len;
    strlcpy(dent->d_name, name, name_size);

    ctx->dirp += size;
    ctx->remaining_count -= size;
    ctx->nwritten += size;
    return true;
}

long file_description_getdents(file_description* desc, void* dirp,
                               unsigned int count) {
    struct inode* inode = desc->inode;
    if (!inode->fops->getdents || !S_ISDIR(inode->mode))
        return -ENOTDIR;

    struct getdents_ctx ctx = {.dirp = dirp,
                               .remaining_count = count,
                               .nwritten = 0,
                               .buffer_is_too_small = false};
    int rc = inode->fops->getdents(&ctx, desc, getdents_callback);
    if (IS_ERR(rc))
        return rc;

    if (ctx.nwritten == 0 && ctx.buffer_is_too_small)
        return -EINVAL;
    return ctx.nwritten;
}

int file_description_block(file_description* desc,
                           bool (*should_unblock)(file_description*)) {
    if ((desc->flags & O_NONBLOCK) && !should_unblock(desc))
        return -EAGAIN;
    return scheduler_block((should_unblock_fn)should_unblock, desc);
}

uint8_t mode_to_dirent_type(mode_t mode) {
    switch (mode & S_IFMT) {
    case S_IFDIR:
        return DT_DIR;
    case S_IFCHR:
        return DT_CHR;
    case S_IFBLK:
        return DT_BLK;
    case S_IFREG:
        return DT_REG;
    case S_IFIFO:
        return DT_FIFO;
    case S_IFLNK:
        return DT_LNK;
    case S_IFSOCK:
        return DT_SOCK;
    }
    UNREACHABLE();
}
