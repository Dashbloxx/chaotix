#include <kernel/api/err.h>
#include <kernel/api/fcntl.h>
#include <kernel/api/sys/stat.h>
#include <kernel/fs/fs.h>
#include <kernel/panic.h>
#include <kernel/process.h>
#include <kernel/system.h>

int sys_open(const char* pathname, int flags, unsigned mode) {
    file_description* desc = vfs_open(pathname, flags, (mode & 0777) | S_IFREG);
    if (IS_ERR(desc))
        return PTR_ERR(desc);
    return process_alloc_file_descriptor(-1, desc);
}

int sys_close(int fd) {
    file_description* desc = process_get_file_description(fd);
    if (IS_ERR(desc))
        return PTR_ERR(desc);

    int rc = file_description_close(desc);
    if (IS_ERR(rc))
        return rc;

    return process_free_file_descriptor(fd);
}

ssize_t sys_read(int fd, void* buf, size_t count) {
    file_description* desc = process_get_file_description(fd);
    if (IS_ERR(desc))
        return PTR_ERR(desc);
    return file_description_read(desc, buf, count);
}

ssize_t sys_write(int fd, const void* buf, size_t count) {
    file_description* desc = process_get_file_description(fd);
    if (IS_ERR(desc))
        return PTR_ERR(desc);
    return file_description_write(desc, buf, count);
}

int sys_ftruncate(int fd, off_t length) {
    file_description* desc = process_get_file_description(fd);
    if (IS_ERR(desc))
        return PTR_ERR(desc);
    return file_description_truncate(desc, length);
}

off_t sys_lseek(int fd, off_t offset, int whence) {
    file_description* desc = process_get_file_description(fd);
    if (IS_ERR(desc))
        return PTR_ERR(desc);
    return file_description_lseek(desc, offset, whence);
}

int sys_stat(const char* pathname, struct stat* buf) {
    return vfs_stat(pathname, buf);
}

int sys_ioctl(int fd, int request, void* argp) {
    file_description* desc = process_get_file_description(fd);
    if (IS_ERR(desc))
        return PTR_ERR(desc);
    return file_description_ioctl(desc, request, argp);
}

int sys_mkdir(const char* pathname, mode_t mode) {
    struct inode* inode = vfs_create(pathname, (mode & 0777) | S_IFDIR);
    if (IS_ERR(inode))
        return PTR_ERR(inode);
    inode_unref(inode);
    return 0;
}

int sys_mknod(const char* pathname, mode_t mode, dev_t dev) {
    switch (mode & S_IFMT) {
    case S_IFREG:
    case S_IFCHR:
    case S_IFBLK:
    case S_IFIFO:
    case S_IFSOCK:
        break;
    default:
        return -EINVAL;
    }
    struct inode* inode = vfs_create(pathname, mode);
    if (IS_ERR(inode))
        return PTR_ERR(inode);
    inode->device_id = dev;
    inode_unref(inode);
    return 0;
}

int sys_link(const char* oldpath, const char* newpath) {
    struct inode* old_inode = vfs_resolve_path(oldpath, NULL, NULL);
    if (IS_ERR(old_inode))
        return PTR_ERR(old_inode);
    if (S_ISDIR(old_inode->mode)) {
        inode_unref(old_inode);
        return -EPERM;
    }

    struct inode* new_parent = NULL;
    char* new_basename = NULL;
    struct inode* new_inode =
        vfs_resolve_path(newpath, &new_parent, &new_basename);
    if (IS_OK(new_inode)) {
        inode_unref(old_inode);
        inode_unref(new_parent);
        inode_unref(new_inode);
        kfree(new_basename);
        return -EEXIST;
    }
    if (IS_ERR(new_inode) && PTR_ERR(new_inode) != -ENOENT) {
        inode_unref(old_inode);
        inode_unref(new_parent);
        kfree(new_basename);
        return PTR_ERR(new_inode);
    }
    if (!new_parent) {
        inode_unref(old_inode);
        kfree(new_basename);
        return -EPERM;
    }
    ASSERT(new_basename);

    int rc = inode_link_child(new_parent, new_basename, old_inode);
    kfree(new_basename);
    return rc;
}

int sys_unlink(const char* pathname) {
    struct inode* parent = NULL;
    char* basename = NULL;
    struct inode* inode = vfs_resolve_path(pathname, &parent, &basename);
    if (IS_ERR(inode)) {
        inode_unref(parent);
        kfree(basename);
        return PTR_ERR(inode);
    }
    if (!parent || S_ISDIR(inode->mode)) {
        inode_unref(parent);
        inode_unref(inode);
        kfree(basename);
        return -EPERM;
    }
    ASSERT(basename);

    inode_unref(inode);
    int rc = inode_unlink_child(parent, basename);
    kfree(basename);
    return rc;
}

static int make_sure_directory_is_empty(struct inode* inode) {
    ASSERT(S_ISDIR(inode->mode));

    file_description* desc = inode_open(inode, O_RDONLY, 0);
    if (IS_ERR(desc))
        return PTR_ERR(desc);

    unsigned char* buf = NULL;
    size_t capacity = 1024;
    ssize_t nread;
    for (;;) {
        buf = krealloc(buf, capacity);
        if (!buf) {
            nread = -ENOMEM;
            break;
        }
        nread = file_description_getdents(desc, buf, capacity);
        if (nread != -EINVAL)
            break;
        capacity *= 2;
    }
    file_description_close(desc);
    kfree(buf);

    return nread > 0 ? -ENOTEMPTY : nread;
}

int sys_rename(const char* oldpath, const char* newpath) {
    int rc = 0;
    struct inode* old_parent = NULL;
    char* old_basename = NULL;
    struct inode* old_inode = NULL;
    struct inode* new_parent = NULL;
    char* new_basename = NULL;
    struct inode* new_inode = NULL;

    old_inode = vfs_resolve_path(oldpath, &old_parent, &old_basename);
    if (IS_ERR(old_inode)) {
        rc = PTR_ERR(old_inode);
        old_inode = NULL;
        goto fail;
    }
    if (!old_parent) {
        rc = -EPERM;
        goto fail;
    }
    ASSERT(old_basename);

    new_inode = vfs_resolve_path(newpath, &new_parent, &new_basename);
    if (IS_OK(new_inode)) {
        if (new_inode == old_inode) {
            rc = 0;
            goto fail;
        }
        if (S_ISDIR(new_inode->mode)) {
            if (!S_ISDIR(old_inode->mode)) {
                rc = -EISDIR;
                goto fail;
            }
            rc = make_sure_directory_is_empty(new_inode);
            if (IS_ERR(rc)) {
                new_inode = NULL;
                goto fail;
            }
        }
        inode_ref(new_parent);
        rc = inode_unlink_child(new_parent, new_basename);
        if (IS_ERR(rc))
            goto fail;
    } else {
        if (PTR_ERR(new_inode) != -ENOENT) {
            rc = PTR_ERR(new_inode);
            new_inode = NULL;
            goto fail;
        }
        new_inode = NULL;
        if (!new_parent) {
            rc = -EPERM;
            goto fail;
        }
    }
    ASSERT(new_basename);

    rc = inode_link_child(new_parent, new_basename, old_inode);
    if (IS_ERR(rc)) {
        old_inode = NULL;
        new_parent = NULL;
        goto fail;
    }
    kfree(new_basename);

    rc = inode_unlink_child(old_parent, old_basename);
    kfree(old_basename);
    return rc;

fail:
    ASSERT(rc < 0);
    inode_unref(old_parent);
    inode_unref(old_inode);
    kfree(old_basename);
    inode_unref(new_parent);
    inode_unref(new_inode);
    kfree(new_basename);
    return rc;
}

int sys_rmdir(const char* pathname) {
    struct inode* parent = NULL;
    char* basename = NULL;
    struct inode* inode = vfs_resolve_path(pathname, &parent, &basename);
    if (IS_ERR(inode)) {
        inode_unref(parent);
        kfree(basename);
        return PTR_ERR(inode);
    }
    if (!parent) {
        inode_unref(inode);
        kfree(basename);
        return -EPERM;
    }
    ASSERT(basename);
    if (!S_ISDIR(inode->mode)) {
        inode_unref(parent);
        inode_unref(inode);
        kfree(basename);
        return -ENOTDIR;
    }
    int rc = make_sure_directory_is_empty(inode);
    if (IS_ERR(rc)) {
        inode_unref(parent);
        kfree(basename);
        return rc;
    }
    rc = inode_unlink_child(parent, basename);
    kfree(basename);
    return rc;
}

long sys_getdents(int fd, void* dirp, size_t count) {
    file_description* desc = process_get_file_description(fd);
    if (IS_ERR(desc))
        return PTR_ERR(desc);
    return file_description_getdents(desc, dirp, count);
}

int sys_fcntl(int fd, int cmd, uintptr_t arg) {
    file_description* desc = process_get_file_description(fd);
    if (IS_ERR(desc))
        return PTR_ERR(desc);
    switch (cmd) {
    case F_DUPFD: {
        int ret = process_alloc_file_descriptor(-1, desc);
        if (IS_ERR(ret))
            return ret;
        ++desc->ref_count;
        return ret;
    }
    case F_GETFL:
        return desc->flags;
    case F_SETFL:
        desc->flags = arg;
        return 0;
    default:
        return -EINVAL;
    }
}

int sys_dup2(int oldfd, int newfd) {
    file_description* oldfd_desc = process_get_file_description(oldfd);
    if (IS_ERR(oldfd_desc))
        return PTR_ERR(oldfd_desc);
    file_description* newfd_desc = process_get_file_description(newfd);
    if (IS_OK(newfd_desc)) {
        int rc = file_description_close(newfd_desc);
        if (IS_ERR(rc))
            return rc;
        rc = process_free_file_descriptor(newfd);
        if (IS_ERR(rc))
            return rc;
    }
    int ret = process_alloc_file_descriptor(newfd, oldfd_desc);
    if (IS_ERR(ret))
        return ret;
    ++oldfd_desc->ref_count;
    return ret;
}

int sys_pipe(int fifofd[2]) {
    struct inode* fifo = fifo_create();
    if (IS_ERR(fifo))
        return PTR_ERR(fifo);

    inode_ref(fifo);
    file_description* reader_desc = inode_open(fifo, O_RDONLY, 0);
    if (IS_ERR(reader_desc)) {
        inode_unref(fifo);
        return PTR_ERR(reader_desc);
    }

    file_description* writer_desc = inode_open(fifo, O_WRONLY, 0);
    if (IS_ERR(writer_desc)) {
        file_description_close(reader_desc);
        return PTR_ERR(writer_desc);
    }

    int reader_fd = process_alloc_file_descriptor(-1, reader_desc);
    if (IS_ERR(reader_fd)) {
        file_description_close(reader_desc);
        file_description_close(writer_desc);
        return reader_fd;
    }

    int writer_fd = process_alloc_file_descriptor(-1, writer_desc);
    if (IS_ERR(writer_fd)) {
        file_description_close(reader_desc);
        file_description_close(writer_desc);
        int rc = process_free_file_descriptor(reader_fd);
        (void)rc;
        return writer_fd;
    }

    fifofd[0] = reader_fd;
    fifofd[1] = writer_fd;

    return 0;
}
