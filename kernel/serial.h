#pragma once

#include "forward.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SERIAL_COM1 0x3f8
#define SERIAL_COM2 0x2f8
#define SERIAL_COM3 0x3e8
#define SERIAL_COM4 0x2e8

void serial_init(void);
bool serial_enable_port(uint16_t port);
size_t serial_write(uint16_t port, const char* s, size_t count);

static inline bool serial_is_valid_port(uint16_t port) {
    switch (port) {
    case SERIAL_COM1:
    case SERIAL_COM2:
    case SERIAL_COM3:
    case SERIAL_COM4:
        return true;
    }
    return false;
}

static inline uint8_t serial_port_to_com_number(uint16_t port) {
    switch (port) {
    case SERIAL_COM1:
        return 1;
    case SERIAL_COM2:
        return 2;
    case SERIAL_COM3:
        return 3;
    case SERIAL_COM4:
        return 4;
    }
    return 0;
}
