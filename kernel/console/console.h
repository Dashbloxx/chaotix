#pragma once

#include <kernel/api/hid.h>
#include <kernel/api/sys/types.h>
#include <stddef.h>

void fb_console_init(void);
void fb_console_on_key(const key_event*);
struct inode* fb_console_device_create(void);

void serial_console_init(void);
void serial_console_on_char(uint16_t port, char);
struct inode* serial_console_device_create(uint16_t port);

void system_console_init(void);
struct inode* system_console_device_create(void);

void tty_maybe_send_signal(pid_t pgid, char ch);
