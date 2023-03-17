#include <common/string.h>
#include <kernel/api/sys/times.h>
#include <kernel/api/sys/wait.h>
#include <kernel/boot_defs.h>
#include <kernel/interrupts.h>
#include <kernel/panic.h>
#include <kernel/process.h>
#include <kernel/scheduler.h>

noreturn uintptr_t sys_exit(int status) { process_exit(status); }

pid_t sys_getpid(void) { return current->pid; }

int sys_setpgid(pid_t pid, pid_t pgid) {
    if (pgid < 0)
        return -EINVAL;

    pid_t target_pid = pid ? pid : current->pid;
    struct process* target = process_find_process_by_pid(target_pid);
    if (!target)
        return -ESRCH;

    target->pgid = pgid ? pgid : target_pid;
    return 0;
}

pid_t sys_getpgid(pid_t pid) {
    if (pid == 0)
        return current->pgid;
    struct process* process = process_find_process_by_pid(pid);
    if (!process)
        return -ESRCH;
    return process->pgid;
}

int sys_sched_yield(void) {
    scheduler_yield(true);
    return 0;
}

void return_to_userland(registers);

pid_t sys_fork(registers* regs) {
    struct process* process =
        kaligned_alloc(alignof(struct process), sizeof(struct process));
    if (!process)
        return -ENOMEM;
    *process = (struct process){0};

    process->pd = paging_clone_current_page_directory();
    if (IS_ERR(process->pd))
        return PTR_ERR(process->pd);

    process->vaddr_allocator = current->vaddr_allocator;

    process->pid = process_generate_next_pid();
    process->ppid = current->pid;
    process->pgid = current->pgid;
    process->eip = (uintptr_t)return_to_userland;
    process->ebx = current->ebx;
    process->esi = current->esi;
    process->edi = current->edi;
    process->fpu_state = current->fpu_state;
    process->state = PROCESS_STATE_RUNNABLE;
    strlcpy(process->comm, current->comm, sizeof(process->comm));

    process->user_ticks = current->user_ticks;
    process->kernel_ticks = current->kernel_ticks;

    process->cwd_path = kstrdup(current->cwd_path);
    if (!process->cwd_path)
        return -ENOMEM;
    process->cwd_inode = current->cwd_inode;
    inode_ref(process->cwd_inode);

    int rc = file_descriptor_table_clone_from(&process->fd_table,
                                              &current->fd_table);
    if (IS_ERR(rc))
        return rc;

    void* stack = kmalloc(STACK_SIZE);
    if (!stack)
        return -ENOMEM;
    process->stack_top = (uintptr_t)stack + STACK_SIZE;
    process->esp = process->ebp = process->stack_top;

    // push the argument of return_to_userland()
    process->esp -= sizeof(registers);
    registers* child_regs = (registers*)process->esp;
    *child_regs = *regs;
    child_regs->eax = 0; // fork() returns 0 in the child

    scheduler_register(process);

    return process->pid;
}

int sys_kill(pid_t pid, int sig) {
    if (pid > 0)
        return process_send_signal_to_one(pid, sig);
    if (pid == 0)
        return process_send_signal_to_group(current->pgid, sig);
    if (pid == -1)
        return process_send_signal_to_all(sig);
    return process_send_signal_to_group(-pid, sig);
}

struct waitpid_blocker {
    pid_t param_pid;
    pid_t current_pid, current_pgid;
    struct process* waited_process;
};

static bool waitpid_should_unblock(struct waitpid_blocker* blocker) {
    bool int_flag = push_cli();

    struct process* prev = NULL;
    struct process* it = all_processes;
    bool any_target_exists = false;

    while (it) {
        bool is_target = false;
        if (blocker->param_pid < -1) {
            if (it->pgid == -blocker->param_pid)
                is_target = true;
        } else if (blocker->param_pid == -1) {
            if (it->ppid == blocker->current_pid)
                is_target = true;
        } else if (blocker->param_pid == 0) {
            if (it->pgid == blocker->current_pgid)
                is_target = true;
        } else {
            if (it->pid == blocker->param_pid)
                is_target = true;
        }
        any_target_exists |= is_target;
        if (is_target && it->state == PROCESS_STATE_DEAD)
            break;

        prev = it;
        it = it->next_in_all_processes;
    }
    if (!it) {
        pop_cli(int_flag);
        if (!any_target_exists) {
            blocker->waited_process = NULL;
            return true;
        }
        return false;
    }

    if (prev)
        prev->next_in_all_processes = it->next_in_all_processes;
    else
        all_processes = it->next_in_all_processes;
    blocker->waited_process = it;

    pop_cli(int_flag);
    return true;
}

pid_t sys_waitpid(pid_t pid, int* wstatus, int options) {
    if (options & ~WNOHANG)
        return -ENOTSUP;

    struct waitpid_blocker blocker = {.param_pid = pid,
                                      .current_pid = current->pid,
                                      .current_pgid = current->pgid,
                                      .waited_process = NULL};
    if (options & WNOHANG) {
        if (!waitpid_should_unblock(&blocker))
            return blocker.waited_process ? 0 : -ECHILD;
    } else {
        int rc = scheduler_block((should_unblock_fn)waitpid_should_unblock,
                                 &blocker);
        if (IS_ERR(rc))
            return rc;
    }

    struct process* waited_process = blocker.waited_process;
    if (!waited_process)
        return -ECHILD;

    scheduler_unregister(waited_process);
    if (wstatus)
        *wstatus = waited_process->exit_status;
    pid_t result = waited_process->pid;
    kfree((void*)(waited_process->stack_top - STACK_SIZE));
    kfree(waited_process);
    return result;
}

clock_t sys_times(struct tms* buf) {
    buf->tms_utime = current->user_ticks;
    buf->tms_stime = current->kernel_ticks;
    return uptime;
}

char* sys_getcwd(char* buf, size_t size) {
    if (!buf || size == 0)
        return ERR_PTR(-EINVAL);
    if (size < strlen(current->cwd_path) + 1)
        return ERR_PTR(-ERANGE);
    strlcpy(buf, current->cwd_path, size);
    return buf;
}

int sys_chdir(const char* path) {
    char* new_cwd_path = vfs_canonicalize_path(path);
    if (IS_ERR(new_cwd_path))
        return PTR_ERR(new_cwd_path);

    struct inode* inode = vfs_resolve_path(path, NULL, NULL);
    if (IS_ERR(inode)) {
        kfree(new_cwd_path);
        return PTR_ERR(inode);
    }
    if (!S_ISDIR(inode->mode)) {
        kfree(new_cwd_path);
        inode_unref(inode);
        return -ENOTDIR;
    }

    kfree(current->cwd_path);
    inode_unref(current->cwd_inode);
    current->cwd_path = new_cwd_path;
    current->cwd_inode = inode;

    return 0;
}
