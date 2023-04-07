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

#include "forward.h"
#include <stdalign.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdnoreturn.h>

#define CLK_TCK 250

/*
 *  Since these registers represent i?86 registers, let's add a macro that checks if we're compiling for i?86. If we are, then let's make a type that represents a
 *  struct which represents the i?86 registers. If we ever support a different architecture, we will create a seperate macro that checks for that architecture, and
 *  defines a seperate struct also named `registers`, that should represent the other architecture's registers.
 */
#if defined(__i386__)
/* Add more macros to create `registers` struct for either i?86, or AMD64/x86_64... */
#if defined(__i386__) && !defined(__x86_64__) 
typedef struct registers {
    uint32_t ss, gs, fs, es, ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t num, err_code;
    uint32_t eip, cs, eflags, user_esp, user_ss;
} __attribute__((packed)) registers;
#elif defined(__x86_64__)
typedef struct registers {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rdi, rsi, rbp, rsp, rbx, rdx, rcx, rax;
    uint64_t int_num, err_code;
    uint64_t rip, cs, rflags, user_rsp, user_ss;
} __attribute__((packed)) registers;
#endif

void dump_registers(const registers*);

struct fpu_state {
    alignas(16) unsigned char buffer[512];
};

void gdt_init(void);
void gdt_set_kernel_stack(uintptr_t stack_top);
#endif

void syscall_init(void);

extern uint32_t uptime;

#if defined(__i386__)
void pit_init(void);
#endif

void cmdline_init(const multiboot_info_t*);
const char* cmdline_get_raw(void);
const char* cmdline_lookup(const char* key);
bool cmdline_contains(const char* key);

void time_init(void);
void time_tick(void);
int time_now(struct timespec*);

noreturn void reboot(void);
noreturn void halt(void);
noreturn void poweroff(void);

struct inode* null_device_create(void);
struct inode* zero_device_create(void);
struct inode* full_device_create(void);

/*
 *  Later on, we may just make macro definitions that allow us to tell the compiler if we want to compile with a specific driver, like the sound driver below. For now
 *  though, we will just check here if we're using the i?86 architecture...
 */
#if defined(__i386__)
bool ac97_init(void);
struct inode* ac97_device_create(void);
#endif