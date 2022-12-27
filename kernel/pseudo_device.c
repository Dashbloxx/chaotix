#include "api/err.h"
#include "api/sys/sysmacros.h"
#include "fs/fs.h"
#include "memory/memory.h"
#include "system.h"
#include <string.h>

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

static ssize_t write_to_bit_bucket(file_description* desc, const void* buffer,
                                   size_t count) {
    (void)desc;
    (void)buffer;
    return count;
}

static ssize_t write_to_full_disk(file_description* desc, const void* buffer,
                                  size_t count) {
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
