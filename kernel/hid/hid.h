#pragma once

#include <kernel/asm_wrapper.h>

#define PS2_DATA 0x60
#define PS2_STATUS 0x64
#define PS2_COMMAND 0x64

#define PS2_READ_CONFIG 0x20
#define PS2_WRITE_CONFIG 0x60
#define PS2_DISABLE_PORT2 0xa7
#define PS2_ENABLE_PORT2 0xa8
#define PS2_DISABLE_PORT1 0xad
#define PS2_ENABLE_PORT1 0xae

#define PS2_ACK 0xfa

#define PS2_INTERRUPT_PORT1 0x1
#define PS2_INTERRUPT_PORT2 0x2

#define PS2_MOUSE_ENABLE_PACKET_STREAMING 0xf4
#define PS2_MOUSE_SET_DEFAULTS 0xf6

static inline uint8_t ps2_read(uint8_t port) {
    #if defined(__i386__)
    while (!(in8(PS2_STATUS) & 1))
        ;
    return in8(port);
    #else
    return 0;
    #endif
}

static inline void ps2_write(uint8_t port, uint8_t data) {
    #if defined(__i386__)
    while (in8(PS2_COMMAND) & 2)
        ;
    out8(port, data);
    #endif
}

void ps2_init(void);
struct inode* ps2_keyboard_device_create(void);
struct inode* ps2_mouse_device_create(void);
