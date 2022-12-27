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
