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

#include "asm_wrapper.h"
#include "interrupts.h"
#include "panic.h"
#include "system.h"
#include <common/extra.h>

/*
 *  This is also i?86-specific code, although there are mechanisms
 *  similar to IRQs that work on other architectures...
 */
#if defined(__i386__)
#define PIC1_CMD 0x20
#define PIC1_DATA 0x21
#define PIC2_CMD 0xa0
#define PIC2_DATA 0xa1

#define PIC_EOI 0x20

#define ICW1_ICW4 0x01
#define ICW1_INIT 0x10
#define ICW4_8086 0x01

#define NUM_IRQS_PER_PIC 8

static void remap_pic(void) {
    out8(PIC1_CMD, ICW1_INIT | ICW1_ICW4);
    out8(PIC2_CMD, ICW1_INIT | ICW1_ICW4);
    out8(PIC1_DATA, IRQ(0));
    out8(PIC2_DATA, IRQ(NUM_IRQS_PER_PIC));
    out8(PIC1_DATA, 4);
    out8(PIC2_DATA, 2);
    out8(PIC1_DATA, ICW4_8086);
    out8(PIC2_DATA, ICW4_8086);
    out8(PIC1_DATA, 0);
    out8(PIC2_DATA, 0);
}

#define ENUMERATE_IRQS(F)                                                      \
    F(0)                                                                       \
    F(1)                                                                       \
    F(2)                                                                       \
    F(3)                                                                       \
    F(4)                                                                       \
    F(5)                                                                       \
    F(6)                                                                       \
    F(7)                                                                       \
    F(8)                                                                       \
    F(9)                                                                       \
    F(10)                                                                      \
    F(11)                                                                      \
    F(12)                                                                      \
    F(13)                                                                      \
    F(14)                                                                      \
    F(15)

#define DEFINE_IRQ(num)                                                        \
    void irq##num(void);                                                       \
    __asm__("irq" #num ":\n"                                                   \
            "pushl $0\n"                                                       \
            "pushl $" STRINGIFY(IRQ(num)) "\n"                                 \
                                          "jmp irq_common_stub");
ENUMERATE_IRQS(DEFINE_IRQ)
#undef DEFINE_IRQ

extern interrupt_handler_fn interrupt_handlers[256];

void irq_handler(registers* regs) {
    ASSERT(!interrupts_enabled());

    if (regs->num >= NUM_IRQS_PER_PIC)
        out8(PIC2_CMD, PIC_EOI);

    out8(PIC1_CMD, PIC_EOI);

    interrupt_handler_fn handler = interrupt_handlers[regs->num];
    if (handler)
        handler(regs);
}

void irq_init(void) {
    remap_pic();

#define REGISTER_IRQ(num)                                                      \
    idt_set_gate(IRQ(num), (uint32_t)irq##num, 0x8, INTERRUPT_GATE32, 0);
    ENUMERATE_IRQS(REGISTER_IRQ)
#undef REGISTER_IRQ

    idt_flush();
}
#endif