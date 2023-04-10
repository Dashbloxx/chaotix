/*
 *  .OOOOOO.   OOOO                                .    O8O              
 *  D8P'  `Y8B  `888                              .O8    `"'              
 * 888           888 .OO.    .OOOO.    .OOOOO.  .O888OO OOOO  OOOO    OOO 
 * 888           888P"Y88B  `P  )88B  D88' `88B   888   `888   `88B..8P'  
 * 888           888   888   .OP"888  888   888   888    888     Y888'    
 * `88B    OOO   888   888  D8(  888  888   888   888 .  888   .O8"'88B   
 *  `Y8BOOD8P'  O888O O888O `Y888""8O `Y8BOD8P'   "888" O888O O88'   888O 
 * 
 *  Chaotix is a UNIX-like operating system that consists of a kernel written in C and
 *  i?86 assembly, and userland binaries written in C.
 *     
 *  Copyright (c) 2023 Nexuss
 *  Copyright (c) 2022 mosm
 *  Copyright (c) 2006-2018 Frans Kaashoek, Robert Morris, Russ Cox, Massachusetts Institute of Technology
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

#include "syscall.h"
#include <kernel/api/sys/reboot.h>
#include <kernel/api/sys/syscall.h>
#include <kernel/api/unistd.h>
#include <kernel/boot_defs.h>
#include <kernel/interrupts.h>
#include <kernel/kprintf.h>
#include <kernel/panic.h>
#include <kernel/process.h>

int sys_reboot(int howto) {
    switch (howto) {
    case RB_AUTOBOOT:
        kputs("Restarting system.\n");
        reboot();
    case RB_HALT:
        kputs("System halted.\n");
        halt();
    case RB_POWEROFF:
        kputs("Power down.\n");
        poweroff();
    default:
        return -1;
    }
}

long sys_sysconf(int name) {
    switch (name) {
    case _SC_MONOTONIC_CLOCK:
        return 1;
    case _SC_OPEN_MAX:
        return OPEN_MAX;
    case _SC_PAGESIZE:
        return PAGE_SIZE;
    case _SC_CLK_TCK:
        return CLK_TCK;
    default:
        return -EINVAL;
    }
}

int sys_dbgputs(const char* str) { return kputs(str); }

typedef uintptr_t (*syscall_handler_fn)(uintptr_t arg1, uintptr_t arg2, uintptr_t arg3, uintptr_t arg4);

static syscall_handler_fn syscall_handlers[NUM_SYSCALLS + 1] = {
#define ENUM_ITEM(name) (syscall_handler_fn)(uintptr_t) sys_##name,
    ENUMERATE_SYSCALLS(ENUM_ITEM)
#undef ENUM_ITEM
        NULL};

static void syscall_handler(registers* regs) {
    ASSERT((regs->cs & 3) == 3);
    ASSERT((regs->ds & 3) == 3);
    ASSERT((regs->es & 3) == 3);
    ASSERT((regs->fs & 3) == 3);
    ASSERT((regs->gs & 3) == 3);
    ASSERT((regs->user_ss & 3) == 3);
    ASSERT(interrupts_enabled());

    process_die_if_needed();

    if (regs->eax >= NUM_SYSCALLS) {
        regs->eax = -ENOSYS;
        return;
    }

    syscall_handler_fn handler = syscall_handlers[regs->eax];
    ASSERT(handler);

    if (regs->eax == SYS_fork)
        regs->eax = handler((uintptr_t)regs, 0, 0, 0);
    else
        regs->eax = handler(regs->edx, regs->ecx, regs->ebx, regs->esi);

    process_die_if_needed();
}

void syscall_init(void) {
    /* IDTs are i?86-specific things... */
    #if defined(__i386__)
    idt_register_interrupt_handler(SYSCALL_VECTOR, syscall_handler);
    idt_set_gate_user_callable(SYSCALL_VECTOR);
    idt_flush();
    #endif
}
