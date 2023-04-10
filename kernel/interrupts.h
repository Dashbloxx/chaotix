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

#pragma once

#include "asm_wrapper.h"
#include "forward.h"
#include <stdbool.h>

/*
 *  Interrupts are not a i?86-specific thing, but the code inside the
 *  preprocessor macro below defines interrupts for an i?86 machine...
 */
#if defined(__i386__)

#define IRQ(i) (0x20 + i)

void idt_init(void);
void irq_init(void);

enum {
    TASK_GATE = 0x5,
    INTERRUPT_GATE16 = 0x6,
    TRAP_GATE16 = 0x7,
    INTERRUPT_GATE32 = 0xe,
    TRAP_GATE32 = 0xf,
};

void idt_set_gate(uint8_t idx, uint32_t base, uint16_t segment_selector, uint8_t gate_type, uint8_t dpl);
void idt_set_gate_user_callable(uint8_t idx);
void idt_flush(void);

typedef void (*interrupt_handler_fn)(registers*);

void idt_register_interrupt_handler(uint8_t num, interrupt_handler_fn handler);

static inline bool interrupts_enabled(void) { return read_eflags() & 0x200; }

static inline bool push_cli(void) {
    bool enabled = interrupts_enabled();
    cli();
    return enabled;
}

static inline void pop_cli(bool was_enabled) {
    if (was_enabled)
        sti();
}

#endif