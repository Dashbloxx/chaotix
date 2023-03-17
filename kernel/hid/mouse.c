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
