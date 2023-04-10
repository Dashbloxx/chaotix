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

#include "console.h"
#include <kernel/api/signum.h>
#include <kernel/api/sys/ioctl.h>
#include <kernel/api/sys/sysmacros.h>
#include <kernel/interrupts.h>
#include <kernel/panic.h>
#include <kernel/process.h>
#include <kernel/ring_buf.h>
#include <kernel/scheduler.h>
#include <kernel/serial.h>

static ring_buf input_bufs[4];
static pid_t pgid;

void serial_console_init(void) {
    for (size_t i = 0; i < 4; ++i)
        ASSERT_OK(ring_buf_init(input_bufs + i));
}

static ring_buf* get_input_buf_for_port(uint16_t port) {
    uint8_t com_number = serial_port_to_com_number(port);
    if (com_number)
        return input_bufs + (com_number - 1);
    return NULL;
}

void serial_console_on_char(uint16_t port, char ch) {
    ring_buf* buf = get_input_buf_for_port(port);
    if (!buf)
        return;

    tty_maybe_send_signal(pgid, ch);

    bool int_flag = push_cli();
    ring_buf_write_evicting_oldest(buf, &ch, 1);
    pop_cli(int_flag);
}

typedef struct serial_console_device {
    struct inode inode;
    uint16_t port;
} serial_console_device;

static bool read_should_unblock(file_description* desc) {
    serial_console_device* dev = (serial_console_device*)desc->inode;
    ring_buf* buf = get_input_buf_for_port(dev->port);
    bool int_flag = push_cli();
    bool should_unblock = !ring_buf_is_empty(buf);
    pop_cli(int_flag);
    return should_unblock;
}

static ssize_t serial_console_device_read(file_description* desc, void* buffer,
                                          size_t count) {
    serial_console_device* dev = (serial_console_device*)desc->inode;
    ring_buf* buf = get_input_buf_for_port(dev->port);

    for (;;) {
        int rc = file_description_block(desc, read_should_unblock);
        if (IS_ERR(rc))
            return rc;

        bool int_flag = push_cli();
        if (ring_buf_is_empty(buf)) {
            pop_cli(int_flag);
            continue;
        }
        ssize_t nread = ring_buf_read(buf, buffer, count);
        pop_cli(int_flag);
        return nread;
    }
}

static ssize_t serial_console_device_write(file_description* desc,
                                           const void* buffer, size_t count) {
    serial_console_device* dev = (serial_console_device*)desc->inode;
    return serial_write(dev->port, (const char*)buffer, count);
}

static int serial_console_device_ioctl(file_description* desc, int request,
                                       void* argp) {
    (void)desc;
    switch (request) {
    case TIOCGPGRP:
        *(pid_t*)argp = pgid;
        return 0;
    case TIOCSPGRP: {
        pid_t new_pgid = *(pid_t*)argp;
        if (new_pgid < 0)
            return -EINVAL;
        pgid = new_pgid;
        return 0;
    }
    case TIOCGWINSZ:
        return -ENOTSUP;
    }
    return -EINVAL;
}

struct inode* serial_console_device_create(uint16_t port) {
    if (!serial_is_valid_port(port))
        return NULL;

    serial_console_device* dev = kmalloc(sizeof(serial_console_device));
    if (!dev)
        return ERR_PTR(-ENOMEM);
    *dev = (serial_console_device){0};

    dev->port = port;

    struct inode* inode = (struct inode*)dev;
    static file_ops fops = {.read = serial_console_device_read,
                            .write = serial_console_device_write,
                            .ioctl = serial_console_device_ioctl};
    inode->fops = &fops;
    inode->mode = S_IFCHR;
    inode->device_id = makedev(4, 63 + (dev_t)serial_port_to_com_number(port));
    inode->ref_count = 1;

    return inode;
}
