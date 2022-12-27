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
