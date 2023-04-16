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

#include "process.h"
#include "api/signum.h"
#include "boot_defs.h"
#include "interrupts.h"
#include "kprintf.h"
#include "memory/memory.h"
#include "panic.h"
#include "scheduler.h"
#include <common/string.h>
#include <stdatomic.h>
#include <common/escp.h>

/* The kernel's process! */
struct process* current;
/* Contains the initial values of the FPU registers... */
struct fpu_state initial_fpu_state;
static atomic_int next_pid = 1;

struct process* all_processes;

extern unsigned char kernel_page_directory[];
extern unsigned char stack_top[];

void process_init(void) {
    /*
     *  Initialize the x87 FPU (Floating Point Unit). According to ChatGPT, when the `fninit` instruction is called, it resets
     *  the FPU registers to their default values...
     *  After that, load the values of the FPU registers to a struct named `initial_fpu_state`. Makes sense right?
     */
    #if defined(__i386__)
    __asm__ volatile("fninit");
    __asm__ volatile("fxsave %0" : "=m"(initial_fpu_state));
    #endif

    current = kaligned_alloc(alignof(struct process), sizeof(struct process));
    ASSERT(current);
    *current = (struct process){0};

    current->fpu_state = initial_fpu_state;
    current->state = PROCESS_STATE_RUNNING;
    strlcpy(current->comm, "kernel_init", sizeof(current->comm));
    current->pd = (page_directory*)((uintptr_t)kernel_page_directory + KERNEL_VADDR);
    current->stack_top = (uintptr_t)stack_top;

    current->cwd_path = kstrdup(ROOT_DIR);
    ASSERT(current->cwd_path);
    current->cwd_inode = vfs_get_root();

    ASSERT_OK(file_descriptor_table_init(&current->fd_table));

    gdt_set_kernel_stack(current->stack_top);
}

struct process* process_create_kernel_process(const char* comm, void (*entry_point)(void)) {
    struct process* process = kaligned_alloc(alignof(struct process), sizeof(struct process));
    if (!process)
        return ERR_PTR(-ENOMEM);
    *process = (struct process){0};

    process->eip = (uintptr_t)entry_point;
    process->fpu_state = initial_fpu_state;
    process->state = PROCESS_STATE_RUNNABLE;
    strlcpy(process->comm, comm, sizeof(process->comm));

    process->pd = paging_create_page_directory();
    if (IS_ERR(process->pd))
        return ERR_CAST(process->pd);

    process->cwd_path = kstrdup(ROOT_DIR);
    if (!process->cwd_path)
        return ERR_PTR(-ENOMEM);
    process->cwd_inode = vfs_get_root();

    int rc = file_descriptor_table_init(&process->fd_table);
    if (IS_ERR(rc))
        return ERR_PTR(rc);

    void* stack = kmalloc(STACK_SIZE);
    if (!stack)
        return ERR_PTR(-ENOMEM);
    process->stack_top = (uintptr_t)stack + STACK_SIZE;
    process->esp = process->ebp = process->stack_top;

    return process;
}

pid_t process_spawn_kernel_process(const char* comm, void (*entry_point)(void)) {
    struct process* process = process_create_kernel_process(comm, entry_point);
    if (IS_ERR(process))
        return PTR_ERR(process);
    scheduler_register(process);
    return process->pid;
}

pid_t process_generate_next_pid(void) { return atomic_fetch_add(&next_pid, 1); }

struct process* process_find_process_by_pid(pid_t pid) {
    bool int_flag = push_cli();
    struct process* it = all_processes;
    for (; it; it = it->next_in_all_processes) {
        if (it->pid == pid)
            break;
    }
    pop_cli(int_flag);
    return it;
}

struct process* process_find_process_by_ppid(pid_t ppid) {
    bool int_flag = push_cli();
    struct process* it = all_processes;
    for (; it; it = it->next_in_all_processes) {
        if (it->ppid == ppid)
            break;
    }
    pop_cli(int_flag);
    return it;
}

static noreturn void die(void) {
    if (current->pid == 1)
        PANIC("init process exited");

    sti();
    paging_destroy_current_page_directory();
    file_descriptor_table_destroy(&current->fd_table);
    kfree(current->cwd_path);
    inode_unref(current->cwd_inode);

    cli();
    {
        struct process* it = all_processes;
        while (it) {
            // Orphaned child process is adopted by init process.
            if (it->ppid == current->pid)
                it->ppid = 1;
            it = it->next_in_all_processes;
        }
    }

    current->state = PROCESS_STATE_DEAD;

    scheduler_yield(false);
    UNREACHABLE();
}

void process_die_if_needed(void) {
    if (current->state == PROCESS_STATE_DYING)
        die();
}

noreturn void process_exit(int status) {
    if (status != 0)
        kprintf("%s%s[%s-%s] %s%sProcess %d exited with status %d%s\n", BOLD, F_CYAN, F_BLUE, F_CYAN, RESET, F_RED, current->pid, status, RESET);
    current->exit_status = (status & 0xff) << 8;
    die();
}

noreturn void process_crash_in_userland(int signum) {
    kprintf("%s%s[%s-%s] %s%sProcess %d crashed with signal %d%s\n", BOLD, F_CYAN, F_BLUE, F_CYAN, RESET, F_RED, current->pid, signum, RESET);
    current->exit_status = signum & 0xff;
    die();
}

static void terminate_with_signal(int signum) {
    kprintf("%s%s[%s-%s] %s%sProcess %d was terminated with signal %d%s\n", BOLD, F_CYAN, F_BLUE, F_CYAN, RESET, F_RED, current->pid, signum, RESET);
    current->exit_status = signum & 0xff;
    current->state = PROCESS_STATE_DYING;
}

void process_tick(bool in_kernel) {
    if (in_kernel)
        ++current->kernel_ticks;
    else
        ++current->user_ticks;
}

int process_alloc_file_descriptor(int fd, file_description* desc) {
    if (fd >= OPEN_MAX)
        return -EBADF;

    if (fd >= 0) {
        file_description** entry = current->fd_table.entries + fd;
        if (*entry)
            return -EEXIST;
        *entry = desc;
        return fd;
    }

    file_description** it = current->fd_table.entries;
    for (int i = 0; i < OPEN_MAX; ++i, ++it) {
        if (*it)
            continue;
        *it = desc;
        return i;
    }
    return -EMFILE;
}

int process_free_file_descriptor(int fd) {
    if (fd < 0 || OPEN_MAX <= fd)
        return -EBADF;

    file_description** desc = current->fd_table.entries + fd;
    if (!*desc)
        return -EBADF;
    *desc = NULL;
    return 0;
}

file_description* process_get_file_description(int fd) {
    if (fd < 0 || OPEN_MAX <= fd)
        return ERR_PTR(-EBADF);

    file_description* desc = current->fd_table.entries[fd];
    if (!desc)
        return ERR_PTR(-EBADF);

    return desc;
}

enum {
    DISP_TERM,
    DISP_IGN,
    DISP_CORE,
    DISP_STOP,
    DISP_CONT,
};

static int get_default_disposition_for_signal(int signum) {
    switch (signum) {
    case SIGHUP:
    case SIGINT:
    case SIGKILL:
    case SIGPIPE:
    case SIGALRM:
    case SIGUSR1:
    case SIGUSR2:
    case SIGVTALRM:
    case SIGSTKFLT:
    case SIGIO:
    case SIGPROF:
    case SIGPWR:
    case SIGTERM:
        return DISP_TERM;
    case SIGCHLD:
    case SIGURG:
    case SIGWINCH:
        return DISP_IGN;
    case SIGQUIT:
    case SIGILL:
    case SIGTRAP:
    case SIGABRT:
    case SIGBUS:
    case SIGFPE:
    case SIGSEGV:
    case SIGXCPU:
    case SIGXFSZ:
    case SIGSYS:
        return DISP_CORE;
    case SIGSTOP:
    case SIGTSTP:
    case SIGTTIN:
    case SIGTTOU:
        return DISP_STOP;
    case SIGCONT:
        return DISP_CONT;
    default:
        UNREACHABLE();
    }
}

static int send_signal(struct process* process, int signum) {
    if (signum < 0 || NSIG <= signum)
        return -EINVAL;

    int disp = get_default_disposition_for_signal(signum);
    switch (disp) {
    case DISP_TERM:
    case DISP_CORE:
        break;
    case DISP_IGN:
        return 0;
    case DISP_STOP:
    case DISP_CONT:
        UNIMPLEMENTED();
    }

    process->pending_signals |= 1 << signum;

    if (process == current)
        process_handle_pending_signals();
    return 0;
}

int process_send_signal_to_one(pid_t pid, int signum) {
    struct process* process = process_find_process_by_pid(pid);
    if (!process)
        return -ESRCH;
    return send_signal(process, signum);
}

int process_send_signal_to_group(pid_t pgid, int signum) {
    bool int_flag = push_cli();
    for (struct process* it = all_processes; it;
         it = it->next_in_all_processes) {
        if (it->pgid != pgid)
            continue;
        int rc = send_signal(it, signum);
        if (IS_ERR(rc)) {
            pop_cli(int_flag);
            return rc;
        }
    }
    pop_cli(int_flag);
    return 0;
}

int process_send_signal_to_all(int signum) {
    bool int_flag = push_cli();
    for (struct process* it = all_processes; it;
         it = it->next_in_all_processes) {
        if (it->pid <= 1)
            continue;
        int rc = send_signal(it, signum);
        if (IS_ERR(rc)) {
            pop_cli(int_flag);
            return rc;
        }
    }
    pop_cli(int_flag);
    return 0;
}

void process_handle_pending_signals(void) {
    if (!current->pending_signals)
        return;

    while (current->pending_signals) {
        int b = __builtin_ffs(current->pending_signals);
        ASSERT(b > 0);
        int signum = b - 1;
        current->pending_signals &= ~(1 << signum);
        int disp = get_default_disposition_for_signal(signum);
        switch (disp) {
        case DISP_TERM:
        case DISP_CORE:
            terminate_with_signal(signum);
            break;
        case DISP_IGN:
            continue;
        case DISP_STOP:
        case DISP_CONT:
            UNIMPLEMENTED();
        }
    }
}
