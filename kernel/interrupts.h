#pragma once

#include "asm_wrapper.h"
#include "forward.h"
#include <stdbool.h>

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

void idt_set_gate(uint8_t idx, uint32_t base, uint16_t segment_selector,
                  uint8_t gate_type, uint8_t dpl);
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
