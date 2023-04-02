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
