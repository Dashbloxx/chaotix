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

#include "fs/fs.h"
#include "memory/memory.h"
#include "system.h"
#include <common/extra.h>
#include <stdnoreturn.h>

struct process {
    pid_t pid, ppid, pgid;
    uint32_t eip, esp, ebp, ebx, esi, edi;
    struct fpu_state fpu_state;

    enum {
        PROCESS_STATE_RUNNABLE,
        PROCESS_STATE_RUNNING,
        PROCESS_STATE_BLOCKED,
        PROCESS_STATE_DYING,
        PROCESS_STATE_DEAD
    } state;
    int exit_status;

    char comm[16];

    page_directory* pd;
    uintptr_t stack_top;
    range_allocator vaddr_allocator;

    char* cwd_path;
    struct inode* cwd_inode;
    file_descriptor_table fd_table;

    bool (*should_unblock)(void*);
    void* blocker_data;
    bool blocker_was_interrupted;

    size_t user_ticks;
    size_t kernel_ticks;

    uint32_t pending_signals;

    struct process* next_in_all_processes;
    struct process* next_in_ready_queue;
};

extern struct process* current;
extern struct process* all_processes;
extern struct fpu_state initial_fpu_state;

void process_init(void);

struct process* process_create_kernel_process(const char* comm,
                                              void (*entry_point)(void));
pid_t process_spawn_kernel_process(const char* comm, void (*entry_point)(void));

pid_t process_generate_next_pid(void);
struct process* process_find_process_by_pid(pid_t);
struct process* process_find_process_by_ppid(pid_t ppid);
noreturn void process_exit(int status);
noreturn void process_crash_in_userland(int signum);

void process_die_if_needed(void);
void process_tick(bool in_kernel);

// if fd < 0, allocates lowest-numbered file descriptor that was unused
NODISCARD int process_alloc_file_descriptor(int fd, file_description*);

NODISCARD int process_free_file_descriptor(int fd);
file_description* process_get_file_description(int fd);

NODISCARD int process_send_signal_to_one(pid_t pid, int signum);
NODISCARD int process_send_signal_to_group(pid_t pgid, int signum);
NODISCARD int process_send_signal_to_all(int signum);
void process_handle_pending_signals(void);
