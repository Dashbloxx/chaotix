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

#include "pci.h"
#include "asm_wrapper.h"

#define PCI_VENDOR_ID 0x0
#define PCI_COMMAND 0x4
#define PCI_DEVICE_ID 0x2
#define PCI_SUBCLASS 0xa
#define PCI_CLASS 0xb
#define PCI_HEADER_TYPE 0xe
#define PCI_BAR0 0x10
#define PCI_BAR1 0x14
#define PCI_SECONDARY_BUS 0x19
#define PCI_INTERRUPT_LINE 0x3c
#define PCI_ADDRESS_PORT 0xcf8
#define PCI_TYPE_BRIDGE 0x604
#define PCI_VALUE_PORT 0xcfc
#define PCI_NONE 0xffff

#define PCI_COMMAND_IO_SPACE 0x1
#define PCI_COMMAND_BUS_MASTER 0x4
#define PCI_COMMAND_INTERRUPT_DISABLE 0x400

static uint32_t io_address_for_field(const struct pci_addr* addr, uint8_t field) {
    return 0x80000000 | ((uint32_t)addr->bus << 16) | ((uint32_t)addr->slot << 11) | ((uint32_t)addr->function << 8) | (field & 0xfc);
}

/* The functions `read_field8`, `read_field16`, `read_field32`, and their equavilents for writing*/

static uint16_t read_field8(const struct pci_addr* addr, uint8_t field) {
    #if defined(__i386__)
    out32(PCI_ADDRESS_PORT, io_address_for_field(addr, field));
    return in8(PCI_VALUE_PORT + (field & 3));
    #else
    return 0;
    #endif
}

static uint16_t read_field16(const struct pci_addr* addr, uint8_t field) {
    #if defined(__i386__)
    out32(PCI_ADDRESS_PORT, io_address_for_field(addr, field));
    return in16(PCI_VALUE_PORT + (field & 2));
    #else
    return 0;
    #endif
}

static uint32_t read_field32(const struct pci_addr* addr, uint8_t field) {
    #if defined(__i386__)
    out32(PCI_ADDRESS_PORT, io_address_for_field(addr, field));
    return in32(PCI_VALUE_PORT);
    #else
    return 0;
    #endif
}

static void write_field16(const struct pci_addr* addr, uint8_t field, uint16_t value) {
    #if defined(__i386__)
    out32(PCI_ADDRESS_PORT, io_address_for_field(addr, field));
    out16(PCI_VALUE_PORT + (field & 2), value);
    #else
    return 0;
    #endif
}

static void enumerate_bus(uint8_t bus, pci_enumeration_callback_fn callback);

static void enumerate_functions(const struct pci_addr* addr, pci_enumeration_callback_fn callback) {
    callback(addr, read_field16(addr, PCI_VENDOR_ID), read_field16(addr, PCI_DEVICE_ID));
    if (pci_get_type(addr) == PCI_TYPE_BRIDGE)
        enumerate_bus(read_field8(addr, PCI_SECONDARY_BUS), callback);
}

static void enumerate_slot(uint8_t bus, uint8_t slot, pci_enumeration_callback_fn callback) {
    #if defined(__i386__)
    struct pci_addr addr = {bus, slot, 0};
    if (read_field16(&addr, PCI_VENDOR_ID) == PCI_NONE)
        return;

    enumerate_functions(&addr, callback);

    if (!(read_field8(&addr, PCI_HEADER_TYPE) & 0x80))
        return;

    for (addr.function = 1; addr.function < 8; ++addr.function) {
        if (read_field16(&addr, PCI_VENDOR_ID) != PCI_NONE)
            enumerate_functions(&addr, callback);
    }
    #endif
}

static void enumerate_bus(uint8_t bus, pci_enumeration_callback_fn callback) {
    for (uint8_t slot = 0; slot < 32; ++slot)
        enumerate_slot(bus, slot, callback);
}

void pci_enumerate(pci_enumeration_callback_fn callback) {
    struct pci_addr addr = {0};
    if ((read_field8(&addr, PCI_HEADER_TYPE) & 0x80) == 0) {
        enumerate_bus(0, callback);
        return;
    }

    for (addr.function = 0; addr.function < 8; ++addr.function) {
        if (read_field16(&addr, PCI_VENDOR_ID) != PCI_NONE)
            enumerate_bus(addr.function, callback);
    }
}

uint16_t pci_get_type(const struct pci_addr* addr) {
    return ((uint16_t)read_field8(addr, PCI_CLASS) << 8) | read_field8(addr, PCI_SUBCLASS);
}

uint32_t pci_get_bar0(const struct pci_addr* addr) {
    return read_field32(addr, PCI_BAR0);
}

uint32_t pci_get_bar1(const struct pci_addr* addr) {
    return read_field32(addr, PCI_BAR1);
}

uint8_t pci_get_interrupt_line(const struct pci_addr* addr) {
    return read_field8(addr, PCI_INTERRUPT_LINE);
}

void pci_set_interrupt_line_enabled(const struct pci_addr* addr, bool enabled) {
    uint16_t command = read_field16(addr, PCI_COMMAND);
    if (enabled)
        command &= ~PCI_COMMAND_INTERRUPT_DISABLE;
    else
        command |= PCI_COMMAND_INTERRUPT_DISABLE;
    write_field16(addr, PCI_COMMAND, command);
}

void pci_set_bus_mastering_enabled(const struct pci_addr* addr, bool enabled) {
    uint16_t command = read_field16(addr, PCI_COMMAND);
    if (enabled)
        command |= PCI_COMMAND_BUS_MASTER;
    else
        command &= ~PCI_COMMAND_BUS_MASTER;
    command |= PCI_COMMAND_IO_SPACE;
    write_field16(addr, PCI_COMMAND, command);
}
