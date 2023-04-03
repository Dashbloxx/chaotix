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

#include "api/time.h"
#include "asm_wrapper.h"
#include "interrupts.h"
#include "panic.h"
#include "process.h"
#include "scheduler.h"
#include "system.h"

#define TIMER0_CTL 0x40
#define PIT_CTL 0x43
#define TIMER0_SELECT 0x00
#define WRITE_WORD 0x30
#define MODE_SQUARE_WAVE 0x06
#define BASE_FREQUENCY 1193182

uint32_t uptime;

static void pit_handler(registers* regs) {
    (void)regs;
    ASSERT(!interrupts_enabled());

    ++uptime;
    time_tick();

    bool in_kernel = (regs->cs & 3) == 0;
    scheduler_tick(in_kernel);
}

void pit_init(void) {
    uint16_t div = BASE_FREQUENCY / CLK_TCK;
    out8(PIT_CTL, TIMER0_SELECT | WRITE_WORD | MODE_SQUARE_WAVE);
    out8(TIMER0_CTL, div & 0xff);
    out8(TIMER0_CTL, div >> 8);
    idt_register_interrupt_handler(IRQ(0), pit_handler);
}
