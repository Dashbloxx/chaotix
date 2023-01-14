#pragma once

#include <stddef.h>
#include <stdint.h>

static uint32_t MMIO_BASE;
 
static inline void mmio_init(int rpi)
{
    switch (rpi) {
        case 2:
        case 3:  MMIO_BASE = 0x3F000000; break;
        case 4:  MMIO_BASE = 0xFE000000; break;
        default: MMIO_BASE = 0x20000000; break;
    }
}
 
static inline void mmio_write(uint32_t reg, uint32_t data)
{
	*(volatile uint32_t*)(MMIO_BASE + reg) = data;
}
 
static inline uint32_t mmio_read(uint32_t reg)
{
	return *(volatile uint32_t*)(MMIO_BASE + reg);
}
 