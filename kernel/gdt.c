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

#include "system.h"
#include <stddef.h>

#if defined(__i386__)
typedef struct gdt_descriptor {
    uint16_t limit_lo : 16;
    uint16_t base_lo : 16;
    uint8_t base_mid : 8;
    uint8_t access : 8;
    uint8_t limit_hi : 4;
    uint8_t flags : 4;
    uint8_t base_hi : 8;
} __attribute__((packed)) gdt_descriptor;

typedef struct gdt_pointer {
    uint16_t limit;
    #if defined(__x86_64__)
    uint64_t base;
    #elif defined(__i386__) && !defined(__x86_64__)
    uint32_t base;
    #endif
} __attribute__((packed)) gdt_pointer;

#if defined(__x86_64__)
struct tss {
    uint32_t prev_tss;
    uint32_t esp0, ss0, esp1, ss1, esp2, ss2;
    uint32_t cr3;
    uint32_t eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi;
    uint32_t es, cs, ss, ds, fs, gs;
    uint32_t ldt;
    uint16_t trap, iomap_base;
} __attribute__((packed));
#elif defined(__i386__) && !defined(__x86_64__)
struct tss {
    uint32_t prev_tss;
    uint32_t reserved0;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved1;
    uint64_t ist1;
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t io_map_base;
} __attribute__((packed));
#endif

#define NUM_GDT_ENTRIES 7
static gdt_descriptor gdt[NUM_GDT_ENTRIES];
static struct tss tss;
static gdt_pointer gdtr;

#if defined(__x86_64__)
static void gdt_set_gate(size_t idx, uint64_t base, uint64_t limit, uint8_t access, uint8_t flags) {
    gdt_descriptor* entry = gdt + idx;

    entry->base_lo = base & 0xffff;
    entry->base_mid = (base >> 16) & 0xff;
    entry->base_hi = (base >> 24) & 0xff;
    entry->base_hi |= (base >> 32) & 0xffffffffull;

    entry->limit_lo = limit & 0xffff;
    entry->limit_hi = (limit >> 16) & 0xf;
    entry->limit_hi |= (limit >> 32) & 0xf0000;

    entry->access = access;
    entry->flags = flags & 0xf;
}
#elif defined(__i386__) && !defined(__x86_64__)
static void gdt_set_gate(size_t idx, uint32_t base, uint32_t limit, uint8_t access, uint8_t flags) {
    gdt_descriptor* entry = gdt + idx;

    entry->base_lo = base & 0xffff;
    entry->base_mid = (base >> 16) & 0xff;
    entry->base_hi = (base >> 24) & 0xff;

    entry->limit_lo = limit & 0xffff;
    entry->limit_hi = (limit >> 16) & 0xf;

    entry->access = access;
    entry->flags = flags & 0xf;
}
#endif

#if defined(__x86_64__)
void gdt_init() {
    gdtr.limit = NUM_GDT_ENTRIES * sizeof(gdt_descriptor) - 1;
    gdtr.base = (uint64_t)&gdt;

    gdt_set_gate(0, 0, 0, 0, 0);
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0x20); // kernel code
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0x00); // kernel data
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0x20); // user code
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0x00); // user data
    gdt_set_gate(5, (uint64_t)&tss, sizeof(struct tss), 0x89, 0x00); // TSS

    tss.io_map_base = sizeof(struct tss);

    // flush GDT
    __asm__ volatile("lgdt %0" ::"m"(gdtr));

    // load kernel data segment
    __asm__ volatile("mov $0x10, %ax\n"
                     "mov %ax, %ds\n"
                     "mov %ax, %es\n"
                     "mov %ax, %fs\n"
                     "mov %ax, %gs\n"
                     "mov %ax, %ss\n");

    // load kernel code segment
    __asm__ volatile("ljmp $0x08, $1f\n"
                     "1:");

    // load TSS segment
    __asm__ volatile("ltr %%ax" ::"a"(0x28));
}
#elif defined(__i386__) && !defined(__x86_64__)
void gdt_init() {
    gdtr.limit = NUM_GDT_ENTRIES * sizeof(gdt_descriptor) - 1;
    gdtr.base = (uint32_t)&gdt;

    gdt_set_gate(0, 0, 0, 0, 0);
    gdt_set_gate(1, 0, 0xfffff, 0x9a, 0xc); // kernel code
    gdt_set_gate(2, 0, 0xfffff, 0x92, 0xc); // kernel data
    gdt_set_gate(3, 0, 0xfffff, 0xfa, 0xc); // user code
    gdt_set_gate(4, 0, 0xfffff, 0xf2, 0xc); // user data
    gdt_set_gate(5, (uint32_t)&tss, sizeof(struct tss), 0xe9, 0); // TSS

    tss.ss0 = 0x10;
    tss.cs = 0x8 | 3;
    tss.ss = tss.ds = tss.es = tss.fs = tss.gs = 0x10 | 3;
    tss.iomap_base = sizeof(struct tss);

    // flush GDT
    __asm__ volatile("lgdt %0\n"
                     "movw %%ax, %%ds\n"
                     "movw %%ax, %%es\n"
                     "movw %%ax, %%fs\n"
                     "movw %%ax, %%gs\n"
                     "movw %%ax, %%ss\n"
                     "ljmpl $0x8, $1f\n"
                     "1:" ::"m"(gdtr),
                     "a"(0x10));

    // flush TSS
    __asm__ volatile("ltr %%ax" ::"a"(0x2b));
}
#endif

#if defined(__x86_64__)
void gdt_set_kernel_stack(uintptr_t stack_top) { tss.rsp[0] = stack_top; }
#elif defined(__i386__) && !defined(__x86_64__)
void gdt_set_kernel_stack(uintptr_t stack_top) { tss.esp0 = stack_top; }
#endif
#endif