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

#include "hid.h"
#include <kernel/api/hid.h>
#include <kernel/api/sys/sysmacros.h>
#include <kernel/fs/fs.h>
#include <kernel/interrupts.h>
#include <kernel/memory/memory.h>
#include <kernel/panic.h>
#include <kernel/scheduler.h>

static void write_mouse(uint8_t data) {
    ps2_write(PS2_COMMAND, 0xd4);
    ps2_write(PS2_DATA, data);
    ASSERT(ps2_read(PS2_DATA) == PS2_ACK);
}

#define QUEUE_SIZE 128

static uint8_t buf[3];
static size_t state = 0;

static mouse_event queue[QUEUE_SIZE];
static size_t queue_read_idx = 0;
static size_t queue_write_idx = 0;

/* IRQs are i?86-specific */
#if defined(__i386__)
static void irq_handler(registers* reg) {
    (void)reg;

    uint8_t data = in8(PS2_DATA);
    buf[state] = data;
    switch (state) {
    case 0:
        ASSERT(data & 8);
        ++state;
        return;
    case 1:
        ++state;
        return;
    case 2: {
        int dx = buf[1];
        int dy = buf[2];
        if (dx && (buf[0] & 0x10))
            dx -= 0x100;
        if (dy && (buf[0] & 0x20))
            dy -= 0x100;
        if (buf[0] & 0xc0)
            dx = dy = 0;

        queue[queue_write_idx] = (mouse_event){dx, -dy, buf[0] & 7};
        queue_write_idx = (queue_write_idx + 1) % QUEUE_SIZE;

        state = 0;
        return;
    }
    }
    UNREACHABLE();
}
#endif

void ps2_mouse_init(void) {
    ps2_write(PS2_COMMAND, PS2_ENABLE_PORT2);
    write_mouse(PS2_MOUSE_SET_DEFAULTS);
    write_mouse(PS2_MOUSE_ENABLE_PACKET_STREAMING);
    idt_register_interrupt_handler(IRQ(12), irq_handler);
}

static bool read_should_unblock(file_description* desc) {
    (void)desc;
    bool int_flag = push_cli();
    bool should_unblock = queue_read_idx != queue_write_idx;
    pop_cli(int_flag);
    return should_unblock;
}

static ssize_t ps2_mouse_device_read(file_description* desc, void* buffer,
                                     size_t count) {
    (void)desc;

    for (;;) {
        int rc = file_description_block(desc, read_should_unblock);
        if (IS_ERR(rc))
            return rc;

        bool int_flag = push_cli();
        if (queue_read_idx == queue_write_idx) {
            pop_cli(int_flag);
            continue;
        }

        size_t nread = 0;
        mouse_event* out = (mouse_event*)buffer;
        while (count > 0) {
            if (queue_read_idx == queue_write_idx ||
                count < sizeof(mouse_event))
                break;
            *out++ = queue[queue_read_idx];
            nread += sizeof(mouse_event);
            count -= sizeof(mouse_event);
            queue_read_idx = (queue_read_idx + 1) % QUEUE_SIZE;
        }
        pop_cli(int_flag);
        return nread;
    }
}

struct inode* ps2_mouse_device_create(void) {
    struct inode* inode = kmalloc(sizeof(struct inode));
    if (!inode)
        return ERR_PTR(-ENOMEM);

    static file_ops fops = {.read = ps2_mouse_device_read};
    *inode = (struct inode){.fops = &fops,
                            .mode = S_IFCHR,
                            .device_id = makedev(10, 1),
                            .ref_count = 1};
    return inode;
}
