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

#pragma once

#include <stdint.h>
#include <stdnoreturn.h>

/* All functions here except `delay` are i?86-exclusive, therefore they are made only available when compiling for i?86... */

#if defined(__i386__)
uint32_t read_eip(void);

static inline uint32_t read_eflags(void) {
    uint32_t eflags;
    __asm__ volatile("pushf;\n"
                     "popl %0"
                     : "=r"(eflags));
    return eflags;
}

static inline void sti(void) { __asm__ volatile("sti"); }

static inline void cli(void) { __asm__ volatile("cli"); }

static inline uint32_t read_cr0(void) {
    uint32_t cr0;
    __asm__("mov %%cr0, %%eax" : "=a"(cr0));
    return cr0;
}

static inline void write_cr0(uint32_t value) {
    __asm__ volatile("mov %%eax, %%cr0" ::"a"(value));
}

static inline uint32_t read_cr2(void) {
    uint32_t cr2;
    __asm__("mov %%cr2, %%eax" : "=a"(cr2));
    return cr2;
}

static inline uint32_t read_cr3(void) {
    uint32_t cr3;
    __asm__ volatile("mov %%cr3, %%eax" : "=a"(cr3));
    return cr3;
}

static inline void write_cr3(uint32_t cr3) {
    __asm__ volatile("mov %%eax, %%cr3" ::"a"(cr3) : "memory");
}

static inline uint32_t read_cr4(void) {
    uint32_t cr4;
    __asm__("mov %%cr4, %%eax" : "=a"(cr4));
    return cr4;
}

static inline void write_cr4(uint32_t value) {
    __asm__ volatile("mov %%eax, %%cr4" ::"a"(value));
}

static inline void flush_tlb(void) { write_cr3(read_cr3()); }

static inline void flush_tlb_single(uintptr_t vaddr) {
    __asm__ volatile("invlpg (%0)" ::"r"(vaddr) : "memory");
}

static inline uint8_t in8(uint16_t port) {
    uint8_t rv;
    __asm__ volatile("inb %1, %0" : "=a"(rv) : "dN"(port));
    return rv;
}

static inline uint16_t in16(uint16_t port) {
    uint16_t rv;
    __asm__ volatile("inw %1, %0" : "=a"(rv) : "dN"(port));
    return rv;
}

static inline uint32_t in32(uint16_t port) {
    uint32_t rv;
    __asm__ volatile("inl %1, %0" : "=a"(rv) : "dN"(port));
    return rv;
}

static inline void out8(uint16_t port, uint8_t data) {
    __asm__ volatile("outb %1, %0" : : "dN"(port), "a"(data));
}

static inline void out16(uint16_t port, uint16_t data) {
    __asm__ volatile("outw %1, %0" : : "dN"(port), "a"(data));
}

static inline void out32(uint16_t port, uint32_t data) {
    __asm__ volatile("outl %1, %0" : : "dN"(port), "a"(data));
}
#endif

static inline void delay(uint32_t usec) {
    #if defined(__i386__)
    uint8_t dummy;
    while (usec--) {
        __asm__ volatile("inb $0x80, %0" : "=a"(dummy));
    }
    #endif
}

static inline void hlt(void) {
    #if defined(__i386__)
    __asm__ volatile("hlt");
    #endif
}

#if defined(__i386__)
static inline noreturn void ud2(void) {
    __asm__ volatile("ud2");
    __builtin_unreachable();
}
#endif

#if defined(__i386__)
static inline void pause(void) { __asm__ volatile("pause"); }
#endif