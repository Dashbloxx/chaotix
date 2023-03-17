#include "api/signum.h"
#include "asm_wrapper.h"
#include "interrupts.h"
#include "isr_stubs.h"
#include "kprintf.h"
#include "panic.h"
#include "process.h"
#include "system.h"

typedef struct idt_descriptor {
    uint16_t base_lo : 16;
    uint16_t segment_selector : 16;
    uint8_t reserved1 : 8;
    uint8_t gate_type : 4;
    bool reserved2 : 1;
    uint8_t dpl : 2;
    bool present : 1;
    uint16_t base_hi : 16;
} __attribute__((packed)) idt_descriptor;

typedef struct idt_pointer {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) idt_pointer;

#define NUM_IDT_ENTRIES 256
static idt_descriptor idt[NUM_IDT_ENTRIES];
static idt_pointer idtr;
interrupt_handler_fn interrupt_handlers[NUM_IDT_ENTRIES];

void idt_register_interrupt_handler(uint8_t num, interrupt_handler_fn handler) {
    interrupt_handlers[num] = handler;
}

void isr_handler(registers* regs) {
    interrupt_handler_fn handler = interrupt_handlers[regs->num];
    if (handler) {
        handler(regs);
        return;
    }

    kprintf("Unhandled interrupt: %u\n", regs->num);
    dump_registers(regs);
    PANIC("Unhandled interrupt");
}

void idt_set_gate(uint8_t idx, uint32_t base, uint16_t segment_selector,
                  uint8_t gate_type, uint8_t dpl) {
    idt_descriptor* entry = idt + idx;

    entry->base_lo = base & 0xffff;
    entry->base_hi = (base >> 16) & 0xffff;

    entry->segment_selector = segment_selector;
    entry->gate_type = gate_type & 0xf;
    entry->dpl = dpl & 3;
    entry->present = true;

    entry->reserved1 = entry->reserved2 = 0;
}

void idt_set_gate_user_callable(uint8_t idx) {
    idt_descriptor* entry = idt + idx;
    entry->gate_type = TRAP_GATE32;
    entry->dpl = 3;
}

void idt_flush(void) { __asm__ volatile("lidt %0" ::"m"(idtr) : "memory"); }

static noreturn void crash(registers* regs, int signum) {
    dump_registers(regs);
    if ((regs->cs & 3) != 3)
        PANIC("Kernel crashed");
    process_crash_in_userland(signum);
}

#define DEFINE_ISR_WITHOUT_ERROR_CODE(num)                                     \
    void isr##num(void);                                                       \
    __asm__("isr" #num ":\n"                                                   \
            "pushl $0\n"                                                       \
            "pushl $" #num "\n"                                                \
            "jmp isr_common_stub");

#define DEFINE_ISR_WITH_ERROR_CODE(num)                                        \
    void isr##num(void);                                                       \
    __asm__("isr" #num ":\n"                                                   \
            "pushl $" #num "\n"                                                \
            "jmp isr_common_stub");

#define DEFINE_EXCEPTION(num, msg)                                             \
    static void handle_exception##num(registers* regs) {                       \
        kprintf("Exception: " msg "\n");                                       \
        dump_registers(regs);                                                  \
        PANIC("Unrecoverable exception");                                      \
    }

#define DEFINE_EXCEPTION_WITHOUT_ERROR_CODE(num, msg)                          \
    DEFINE_ISR_WITHOUT_ERROR_CODE(num)                                         \
    DEFINE_EXCEPTION(num, msg)

#define DEFINE_EXCEPTION_WITH_ERROR_CODE(num, msg)                             \
    DEFINE_ISR_WITH_ERROR_CODE(num)                                            \
    DEFINE_EXCEPTION(num, msg)

DEFINE_ISR_WITHOUT_ERROR_CODE(0)
static void handle_exception0(registers* regs) {
    kprintf("Divide-by-zero error\n");
    crash(regs, SIGFPE);
}

DEFINE_EXCEPTION_WITHOUT_ERROR_CODE(1, "Debug")
DEFINE_EXCEPTION_WITHOUT_ERROR_CODE(2, "Non-maskable interrupt")
DEFINE_EXCEPTION_WITHOUT_ERROR_CODE(3, "Breakpoint")
DEFINE_EXCEPTION_WITHOUT_ERROR_CODE(4, "Overflow")
DEFINE_EXCEPTION_WITHOUT_ERROR_CODE(5, "Bound range exceeded")

DEFINE_ISR_WITHOUT_ERROR_CODE(6)
static void handle_exception6(registers* regs) {
    kprintf("Invalid opcode\n");
    crash(regs, SIGILL);
}

DEFINE_EXCEPTION_WITHOUT_ERROR_CODE(7, "Device not available")
DEFINE_EXCEPTION_WITH_ERROR_CODE(8, "Double fault")
DEFINE_EXCEPTION_WITHOUT_ERROR_CODE(9, "Coprocessor segment overrun")
DEFINE_EXCEPTION_WITH_ERROR_CODE(10, "Invalid TSS")
DEFINE_EXCEPTION_WITH_ERROR_CODE(11, "Segment not present")
DEFINE_EXCEPTION_WITH_ERROR_CODE(12, "Stack-segment fault")

DEFINE_ISR_WITH_ERROR_CODE(13)
static void handle_exception13(registers* regs) {
    kprintf("General protection fault\n");
    crash(regs, SIGSEGV);
}

DEFINE_ISR_WITH_ERROR_CODE(14)
static void handle_exception14(registers* regs) {
    uint32_t present = regs->err_code & 0x1;
    uint32_t write = regs->err_code & 0x2;
    uint32_t user = regs->err_code & 0x4;

    kprintf("Page fault (%s%s%s) at 0x%x\n",
            present ? "page-protection " : "non-present ",
            write ? "write " : "read ", user ? "user-mode" : "kernel-mode",
            read_cr2());
    crash(regs, SIGSEGV);
}

DEFINE_EXCEPTION_WITHOUT_ERROR_CODE(15, "Unknown")
DEFINE_EXCEPTION_WITHOUT_ERROR_CODE(16, "x87 floating-point exception")

ENUMERATE_ISR_STUBS(DEFINE_ISR_WITHOUT_ERROR_CODE)

void idt_init(void) {
    idtr.limit = NUM_IDT_ENTRIES * sizeof(idt_descriptor) - 1;
    idtr.base = (uint32_t)&idt;

#define REGISTER_ISR(num)                                                      \
    idt_set_gate(num, (uint32_t)isr##num, 0x8, INTERRUPT_GATE32, 0);

#define REGISTER_EXCEPTION(num)                                                \
    REGISTER_ISR(num);                                                         \
    idt_register_interrupt_handler(num, handle_exception##num);

    REGISTER_EXCEPTION(0);
    REGISTER_EXCEPTION(1);
    REGISTER_EXCEPTION(2);
    REGISTER_EXCEPTION(3);
    REGISTER_EXCEPTION(4);
    REGISTER_EXCEPTION(5);
    REGISTER_EXCEPTION(6);
    REGISTER_EXCEPTION(7);
    REGISTER_EXCEPTION(8);
    REGISTER_EXCEPTION(9);
    REGISTER_EXCEPTION(10);
    REGISTER_EXCEPTION(11);
    REGISTER_EXCEPTION(12);
    REGISTER_EXCEPTION(13);
    REGISTER_EXCEPTION(14);
    REGISTER_EXCEPTION(15);
    REGISTER_EXCEPTION(16);

    ENUMERATE_ISR_STUBS(REGISTER_ISR)

    idt_flush();
}
