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
#include <kernel/api/sys/sysmacros.h>
#include <kernel/console/console.h>
#include <kernel/fs/fs.h>
#include <kernel/interrupts.h>
#include <kernel/memory/memory.h>
#include <kernel/panic.h>
#include <kernel/scheduler.h>

#define QUEUE_SIZE 128

// en-US
static const char scancode_to_key[256] = {
    0,   '\x1b', '1',  '2', '3',  '4', '5', '6', '7', '8', '9', '0', '-',
    '=', '\b',   '\t', 'q', 'w',  'e', 'r', 't', 'y', 'u', 'i', 'o', 'p',
    '[', ']',    '\n', 0,   'a',  's', 'd', 'f', 'g', 'h', 'j', 'k', 'l',
    ';', '\'',   '`',  0,   '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',',
    '.', '/',    0,    '*', 0,    ' ', 0,   0,   0,   0,   0,   0,   0,
    0,   0,      0,    0,   0,    0,   '7', '8', '9', '-', '4', '5', '6',
    '+', '1',    '2',  '3', '0',  '.', 0,   0,   '\\'};
static const char scancode_to_shifted_key[256] = {
    0,   '\x1b', '!',  '@', '#', '$', '%', '^', '&', '*', '(', ')', '_',
    '+', '\b',   '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P',
    '{', '}',    '\n', 0,   'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L',
    ':', '"',    '~',  0,   '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<',
    '>', '?',    0,    '*', 0,   ' ', 0,   0,   0,   0,   0,   0,   0,
    0,   0,      0,    0,   0,   0,   '7', '8', '9', '-', '4', '5', '6',
    '+', '1',    '2',  '3', '0', '.', 0,   0,   '|'};

static const uint8_t scancode_to_keycode[256] = {

    KEYCODE_NONE,
    KEYCODE_ESCAPE,
    KEYCODE_1,
    KEYCODE_2,
    KEYCODE_3,
    KEYCODE_4,
    KEYCODE_5,
    KEYCODE_6,
    KEYCODE_7,
    KEYCODE_8,
    KEYCODE_9,
    KEYCODE_0,
    KEYCODE_MINUS,
    KEYCODE_EQUALS,
    KEYCODE_BACKSPACE,
    KEYCODE_TAB,
    KEYCODE_Q,
    KEYCODE_W,
    KEYCODE_E,
    KEYCODE_R,
    KEYCODE_T,
    KEYCODE_Y,
    KEYCODE_U,
    KEYCODE_I,
    KEYCODE_O,
    KEYCODE_P,
    KEYCODE_LEFT_BRACKET,
    KEYCODE_RIGHT_BRACKET,
    KEYCODE_RETURN,
    KEYCODE_CONTROL,
    KEYCODE_A,
    KEYCODE_S,
    KEYCODE_D,
    KEYCODE_F,
    KEYCODE_G,
    KEYCODE_H,
    KEYCODE_J,
    KEYCODE_K,
    KEYCODE_L,
    KEYCODE_SEMICOLON,
    KEYCODE_QUOTE,
    KEYCODE_BACK_QUOTE,
    KEYCODE_LEFT_SHIFT,
    KEYCODE_BACKSLASH,
    KEYCODE_Z,
    KEYCODE_X,
    KEYCODE_C,
    KEYCODE_V,
    KEYCODE_B,
    KEYCODE_N,
    KEYCODE_M,
    KEYCODE_COMMA,
    KEYCODE_PERIOD,
    KEYCODE_SLASH,
    KEYCODE_RIGHT_SHIFT,
    KEYCODE_ASTERISK,
    KEYCODE_ALT,
    KEYCODE_SPACE,
    KEYCODE_CAPS_LOCK,
    KEYCODE_F1,
    KEYCODE_F2,
    KEYCODE_F3,
    KEYCODE_F4,
    KEYCODE_F5,
    KEYCODE_F6,
    KEYCODE_F7,
    KEYCODE_F8,
    KEYCODE_F9,
    KEYCODE_F10,
    KEYCODE_NUMLOCK,
    KEYCODE_NONE,
    KEYCODE_HOME,
    KEYCODE_UP,
    KEYCODE_PAGE_UP,
    KEYCODE_MINUS,
    KEYCODE_LEFT,
    KEYCODE_NONE,
    KEYCODE_RIGHT,
    KEYCODE_PLUS,
    KEYCODE_END,
    KEYCODE_DOWN,
    KEYCODE_PAGE_DOWN,
    KEYCODE_NONE,
    KEYCODE_DELETE,
    KEYCODE_NONE,
    KEYCODE_NONE,
    KEYCODE_BACKSLASH,
    KEYCODE_F11,
    KEYCODE_F12,
    KEYCODE_NONE,
    KEYCODE_NONE,
    KEYCODE_SUPER,
    KEYCODE_NONE,
    KEYCODE_MENU};

static const uint8_t scancode_to_shifted_keycode[256] = {

    KEYCODE_NONE,
    KEYCODE_ESCAPE,
    KEYCODE_EXCLAIM,
    KEYCODE_AT,
    KEYCODE_HASH,
    KEYCODE_DOLLAR,
    KEYCODE_PERCENT,
    KEYCODE_CARET,
    KEYCODE_AMPERSAND,
    KEYCODE_ASTERISK,
    KEYCODE_LEFT_PAREN,
    KEYCODE_RIGHT_PAREN,
    KEYCODE_UNDERSCORE,
    KEYCODE_PLUS,
    KEYCODE_BACKSPACE,
    KEYCODE_TAB,
    KEYCODE_Q,
    KEYCODE_W,
    KEYCODE_E,
    KEYCODE_R,
    KEYCODE_T,
    KEYCODE_Y,
    KEYCODE_U,
    KEYCODE_I,
    KEYCODE_O,
    KEYCODE_P,
    KEYCODE_LEFT_CURLY_BRACKET,
    KEYCODE_RIGHT_CURLY_BRACKET,
    KEYCODE_RETURN,
    KEYCODE_CONTROL,
    KEYCODE_A,
    KEYCODE_S,
    KEYCODE_D,
    KEYCODE_F,
    KEYCODE_G,
    KEYCODE_H,
    KEYCODE_J,
    KEYCODE_K,
    KEYCODE_L,
    KEYCODE_COLON,
    KEYCODE_DOUBLE_QUOTE,
    KEYCODE_TILDE,
    KEYCODE_LEFT_SHIFT,
    KEYCODE_PIPE,
    KEYCODE_Z,
    KEYCODE_X,
    KEYCODE_C,
    KEYCODE_V,
    KEYCODE_B,
    KEYCODE_N,
    KEYCODE_M,
    KEYCODE_LESS,
    KEYCODE_GREATER,
    KEYCODE_QUESTION,
    KEYCODE_RIGHT_SHIFT,
    KEYCODE_ASTERISK,
    KEYCODE_ALT,
    KEYCODE_SPACE,
    KEYCODE_CAPS_LOCK,
    KEYCODE_F1,
    KEYCODE_F2,
    KEYCODE_F3,
    KEYCODE_F4,
    KEYCODE_F5,
    KEYCODE_F6,
    KEYCODE_F7,
    KEYCODE_F8,
    KEYCODE_F9,
    KEYCODE_F10,
    KEYCODE_NUMLOCK,
    KEYCODE_NONE,
    KEYCODE_HOME,
    KEYCODE_UP,
    KEYCODE_PAGE_UP,
    KEYCODE_MINUS,
    KEYCODE_LEFT,
    KEYCODE_NONE,
    KEYCODE_RIGHT,
    KEYCODE_PLUS,
    KEYCODE_END,
    KEYCODE_DOWN,
    KEYCODE_PAGE_DOWN,
    KEYCODE_NONE,
    KEYCODE_DELETE,
    KEYCODE_NONE,
    KEYCODE_NONE,
    KEYCODE_PIPE,
    KEYCODE_F11,
    KEYCODE_F12,
    KEYCODE_NONE,
    KEYCODE_NONE,
    KEYCODE_SUPER,
    KEYCODE_NONE,
    KEYCODE_MENU,
};

static bool received_e0 = false;

#define STATE_ALT 0x1
#define STATE_CTRL 0x2
#define STATE_LEFT_SHIFT 0x4
#define STATE_RIGHT_SHIFT 0x8
#define STATE_LEFT_SUPER 0x10
#define STATE_RIGHT_SUPER 0x20
#define STATE_ALTGR 0x40

static uint8_t state = 0;
static void set_state(uint8_t which, bool pressed) {
    if (pressed)
        state |= which;
    else
        state &= ~which;
}

static key_event queue[QUEUE_SIZE];
static size_t queue_read_idx = 0;
static size_t queue_write_idx = 0;

static void irq_handler(registers* reg) {
    (void)reg;

    uint8_t data = in8(PS2_DATA);
    if (data == 0xe0) {
        received_e0 = true;
        return;
    }

    unsigned char ch = data & 0x7f;
    bool pressed = !(data & 0x80);

    switch (ch) {
    case 0x38:
        set_state(received_e0 ? STATE_ALTGR : STATE_ALT, pressed);
        break;
    case 0x1d:
        set_state(STATE_CTRL, pressed);
        break;
    case 0x5b:
        set_state(STATE_LEFT_SUPER, pressed);
        break;
    case 0x5c:
        set_state(STATE_RIGHT_SUPER, pressed);
        break;
    case 0x2a:
        set_state(STATE_LEFT_SHIFT, pressed);
        break;
    case 0x36:
        set_state(STATE_RIGHT_SHIFT, pressed);
        break;
    }

    uint8_t modifiers = 0;
    if (state & STATE_ALT)
        modifiers |= KEY_MODIFIER_ALT;
    if (state & STATE_CTRL)
        modifiers |= KEY_MODIFIER_CTRL;
    if (state & (STATE_LEFT_SHIFT | STATE_RIGHT_SHIFT))
        modifiers |= KEY_MODIFIER_SHIFT;
    if (state & (STATE_LEFT_SUPER | STATE_RIGHT_SUPER))
        modifiers |= KEY_MODIFIER_SUPER;
    if (state & STATE_ALTGR)
        modifiers |= KEY_MODIFIER_ALTGR;

    const char* to_key = (modifiers & KEY_MODIFIER_SHIFT) ? scancode_to_shifted_key : scancode_to_key;
    const uint8_t* to_keycode = (modifiers & KEY_MODIFIER_SHIFT) ? scancode_to_shifted_keycode : scancode_to_keycode;

    key_event* event = queue + queue_write_idx;
    queue_write_idx = (queue_write_idx + 1) % QUEUE_SIZE;

    event->scancode = ch;
    if (received_e0)
        event->scancode |= 0xe000;
    event->key = to_key[ch];
    event->keycode = to_keycode[ch];
    event->modifiers = modifiers;
    event->pressed = pressed;

    received_e0 = false;

    fb_console_on_key(event);
}

void ps2_keyboard_init(void) {
    ps2_write(PS2_COMMAND, PS2_ENABLE_PORT1);
    #if defined(__i386__)
    idt_register_interrupt_handler(IRQ(1), irq_handler);
    #endif
}

static bool read_should_unblock(file_description* desc) {
    #if defined(__i386__)
    (void)desc;
    bool int_flag = push_cli();
    bool should_unblock = queue_read_idx != queue_write_idx;
    pop_cli(int_flag);
    return should_unblock;
    #else
    return false;
    #endif
}

static ssize_t ps2_keyboard_device_read(file_description* desc, void* buffer, size_t count) {
    #if defined(__i386__)
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
        key_event* out = (key_event*)buffer;
        while (count > 0) {
            if (queue_read_idx == queue_write_idx || count < sizeof(key_event))
                break;
            *out++ = queue[queue_read_idx];
            nread += sizeof(key_event);
            count -= sizeof(key_event);
            queue_read_idx = (queue_read_idx + 1) % QUEUE_SIZE;
        }
        pop_cli(int_flag);
        return nread;
    }
    #else
    return 0;
    #endif
}

struct inode* ps2_keyboard_device_create(void) {
    struct inode* inode = kmalloc(sizeof(struct inode));
    if (!inode)
        return ERR_PTR(-ENOMEM);

    static file_ops fops = {.read = ps2_keyboard_device_read};
    *inode = (struct inode){.fops = &fops, .mode = S_IFCHR, .device_id = makedev(11, 0), .ref_count = 1};
    return inode;
}
