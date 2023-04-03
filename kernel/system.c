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
#include "asm_wrapper.h"
#include "hid/hid.h"
#include "kprintf.h"
#include "panic.h"
#include <common/string.h>

noreturn void reboot(void) {
    out8(PS2_COMMAND, 0xfe);
    halt();
}

noreturn void halt(void) {
    cli();
    for (;;)
        hlt();
}

noreturn void poweroff(void) {
    // this works only on emulators
    out16(0x604, 0x2000);  // QEMU
    out16(0x4004, 0x3400); // Virtualbox
    out16(0xb004, 0x2000); // Bochs and older versions of QEMU
    halt();
}

noreturn void panic(const char* message, const char* file, size_t line) {
    kprintf("%s at %s:%u\n", message, file, line);

    const char* mode = cmdline_lookup("panic");
    if (mode) {
        if (!strcmp(mode, "poweroff"))
            poweroff();
    }
    halt();
}

void dump_registers(const registers* regs) {
    uint16_t ss;
    uint32_t esp;
    if (regs->cs & 3) {
        ss = regs->user_ss;
        esp = regs->user_esp;
    } else {
        ss = regs->ss;
        esp = regs->esp;
    }
    kprintf("  num=%u err_code=0x%08x\n"
            "   pc=0x%04x:0x%08x eflags=0x%08x\n"
            "stack=0x%04x:0x%08x\n"
            "   ds=0x%04x es=0x%04x fs=0x%04x gs=0x%04x\n"
            "  eax=0x%08x ebx=0x%08x ecx=0x%08x edx=0x%08x\n"
            "  ebp=0x%08x esp=0x%08x esi=0x%08x edi=0x%08x\n"
            "  cr0=0x%08x cr2=0x%08x cr3=0x%08x cr4=0x%08x\n",
            regs->num, regs->err_code, regs->cs, regs->eip, regs->eflags, ss,
            esp, regs->ds, regs->es, regs->fs, regs->gs, regs->eax, regs->ebx,
            regs->ecx, regs->edx, regs->ebp, regs->esp, regs->esi, regs->edi,
            read_cr0(), read_cr2(), read_cr3(), read_cr4());
}
