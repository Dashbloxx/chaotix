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
