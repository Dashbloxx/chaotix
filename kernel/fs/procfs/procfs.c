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
