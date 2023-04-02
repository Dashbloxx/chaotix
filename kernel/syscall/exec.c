#include <common/extra.h>
#include <common/libgen.h>
#include <common/string.h>
#include <kernel/api/elf.h>
#include <kernel/api/fcntl.h>
#include <kernel/asm_wrapper.h>
#include <kernel/boot_defs.h>
#include <kernel/panic.h>
#include <kernel/process.h>
//#include <string.h>

typedef struct string_list {
    size_t count;
    char* buf;
    char** elements;
} string_list;

static int string_list_create(string_list* strings, char* const src[]) {
    strings->count = 0;
    size_t total_size = 0;
    for (char* const* it = src; *it; ++it) {
        total_size += strlen(*it) + 1;
        ++strings->count;
    }

    if (strings->count == 0) {
        strings->buf = NULL;
        strings->elements = NULL;
        return 0;
    }

    strings->buf = kmalloc(total_size);
    if (!strings->buf)
        return -ENOMEM;

    strings->elements = kmalloc(strings->count * sizeof(char*));
    if (!strings->elements) {
        kfree(strings->buf);
        strings->buf = NULL;
        return -ENOMEM;
    }

    char* cursor = strings->buf;
    for (size_t i = 0; i < strings->count; ++i) {
        size_t size = strlen(src[i]) + 1;
        strlcpy(cursor, src[i], size);
        strings->elements[i] = cursor;
        cursor += size;
    }

    return 0;
}

static void string_list_destroy(string_list* strings) {
    if (strings->buf) {
        kfree(strings->buf);
        strings->buf = NULL;
    }
    if (strings->elements) {
        kfree(strings->elements);
        strings->elements = NULL;
    }
}

typedef struct ptr_list {
    size_t count;
    uintptr_t* elements;
} ptr_list;

static void ptr_list_destroy(ptr_list* ptrs) {
    if (ptrs->elements) {
        kfree(ptrs->elements);
        ptrs->elements = NULL;
    }
}

NODISCARD static int push_value(uintptr_t* sp, uintptr_t stack_base,
                                uintptr_t value) {
    if (*sp - sizeof(uintptr_t) < stack_base)
        return -E2BIG;
    *sp -= sizeof(uintptr_t);
    *(uintptr_t*)*sp = value;
    return 0;
}

NODISCARD static int push_strings(uintptr_t* sp, uintptr_t stack_base,
                                  ptr_list* ptrs, const string_list* strings) {
    ptrs->count = strings->count;

    if (strings->count == 0) {
        ptrs->elements = NULL;
        return 0;
    }

    ptrs->elements = kmalloc(strings->count * sizeof(uintptr_t));
    if (!ptrs->elements)
        return -ENOMEM;

    for (size_t i = 0; i < strings->count; ++i) {
        size_t size = strlen(strings->elements[i]) + 1;
        if (*sp - size < stack_base) {
            kfree(ptrs->elements);
            ptrs->elements = NULL;
            return -E2BIG;
        }
        *sp -= next_power_of_two(size);
        strlcpy((char*)*sp, strings->elements[i], size);
        ptrs->elements[ptrs->count - i - 1] = *sp;
    }

    return 0;
}

NODISCARD static int push_ptrs(uintptr_t* sp, uintptr_t stack_base,
                               const ptr_list* ptrs) {
    for (size_t i = 0; i < ptrs->count; ++i) {
        int rc = push_value(sp, stack_base, ptrs->elements[i]);
        if (IS_ERR(rc))
            return rc;
    }
    return 0;
}

int sys_execve(const char* pathname, char* const argv[], char* const envp[]) {
    /* Contains i?86-specific code, so we'll add guards that check if we're compiling for i?86... */
    #if defined(__i386__)
    if (!pathname || !argv || !envp)
        return -EFAULT;

    struct stat stat;
    int rc = vfs_stat(pathname, &stat);
    if (IS_ERR(rc))
        return rc;
    if (!S_ISREG(stat.st_mode))
        return -EACCES;
    if ((size_t)stat.st_size < sizeof(Elf32_Ehdr))
        return -ENOEXEC;

    char* dup_pathname = kstrdup(pathname);
    if (!dup_pathname)
        return -ENOMEM;
    const char* exe_basename = basename(dup_pathname);
    char comm[sizeof(current->comm)];
    strlcpy(comm, exe_basename, sizeof(current->comm));
    kfree(dup_pathname);

    file_description* desc = vfs_open(pathname, O_RDONLY, 0);
    if (IS_ERR(desc))
        return PTR_ERR(desc);

    void* executable_buf = kmalloc(stat.st_size);
    if (!executable_buf) {
        file_description_close(desc);
        return -ENOMEM;
    }
    ssize_t nread = file_description_read(desc, executable_buf, stat.st_size);
    file_description_close(desc);
    if (IS_ERR(nread)) {
        kfree(executable_buf);
        return nread;
    }

    Elf32_Ehdr* ehdr = (Elf32_Ehdr*)executable_buf;
    if (!IS_ELF(*ehdr) || ehdr->e_ident[EI_CLASS] != ELFCLASS32 ||
        ehdr->e_ident[EI_DATA] != ELFDATA2LSB ||
        ehdr->e_ident[EI_VERSION] != EV_CURRENT ||
        ehdr->e_ident[EI_OSABI] != ELFOSABI_SYSV ||
        ehdr->e_ident[EI_ABIVERSION] != 0 || ehdr->e_machine != EM_386 ||
        ehdr->e_type != ET_EXEC || ehdr->e_version != EV_CURRENT) {
        kfree(executable_buf);
        return -ENOEXEC;
    }

    // after switching page directory we will no longer be able to access
    // argv and envp, so we copy them here.
    string_list copied_argv = (string_list){0};
    rc = string_list_create(&copied_argv, argv);
    if (IS_ERR(rc)) {
        kfree(executable_buf);
        return rc;
    }

    string_list copied_envp = (string_list){0};
    rc = string_list_create(&copied_envp, envp);
    if (IS_ERR(rc)) {
        kfree(executable_buf);
        string_list_destroy(&copied_argv);
        return rc;
    }

    page_directory* prev_pd = paging_current_page_directory();

    page_directory* new_pd = paging_create_page_directory();
    if (IS_ERR(new_pd)) {
        kfree(executable_buf);
        string_list_destroy(&copied_argv);
        string_list_destroy(&copied_envp);
        return PTR_ERR(new_pd);
    }

    paging_switch_page_directory(new_pd);

    // after this point, we have to revert to prev_pd if we want to abort.

    int ret = 0;
    ptr_list envp_ptrs = (ptr_list){0};
    ptr_list argv_ptrs = (ptr_list){0};

    Elf32_Phdr* phdr = (Elf32_Phdr*)((uintptr_t)executable_buf + ehdr->e_phoff);
    uintptr_t max_segment_addr = 0;
    for (size_t i = 0; i < ehdr->e_phnum; ++i, ++phdr) {
        if (phdr->p_type != PT_LOAD)
            continue;
        if (phdr->p_filesz > phdr->p_memsz) {
            ret = -ENOEXEC;
            goto fail;
        }

        uintptr_t region_start = round_down(phdr->p_vaddr, PAGE_SIZE);
        uintptr_t region_end = round_up(phdr->p_vaddr + phdr->p_memsz, PAGE_SIZE);
        ret = paging_map_to_free_pages(region_start, region_end - region_start, PAGE_USER | PAGE_WRITE);
        if (IS_ERR(ret))
            goto fail;

        memset((void*)region_start, 0, phdr->p_vaddr - region_start);
        memcpy((void*)phdr->p_vaddr, (void*)((uintptr_t)executable_buf + phdr->p_offset), phdr->p_filesz);
        memset((void*)(phdr->p_vaddr + phdr->p_filesz), 0, region_end - phdr->p_vaddr - phdr->p_filesz);

        if (max_segment_addr < region_end)
            max_segment_addr = region_end;
    }

    uint32_t entry_point = ehdr->e_entry;
    kfree(executable_buf);
    executable_buf = NULL;

    range_allocator vaddr_allocator;
    ret = range_allocator_init(&vaddr_allocator, max_segment_addr, KERNEL_VADDR);
    if (IS_ERR(ret))
        goto fail;

    // we keep extra pages before and after stack unmapped to detect stack
    // overflow and underflow by causing page faults
    uintptr_t stack_region = range_allocator_alloc(&vaddr_allocator, 2 * PAGE_SIZE + STACK_SIZE);
    if (IS_ERR(stack_region)) {
        ret = stack_region;
        goto fail;
    }
    uintptr_t stack_base = stack_region + PAGE_SIZE;
    ret = paging_map_to_free_pages(stack_base, STACK_SIZE, PAGE_WRITE | PAGE_USER);
    if (IS_ERR(ret))
        goto fail;

    uintptr_t sp = stack_base + STACK_SIZE;
    memset((void*)stack_base, 0, STACK_SIZE);

    int argc = copied_argv.count;

    ret = push_strings(&sp, stack_base, &envp_ptrs, &copied_envp);
    string_list_destroy(&copied_envp);
    if (IS_ERR(ret))
        goto fail;

    ret = push_strings(&sp, stack_base, &argv_ptrs, &copied_argv);
    string_list_destroy(&copied_argv);
    if (IS_ERR(ret))
        goto fail;

    ret = push_value(&sp, stack_base, 0);
    if (IS_ERR(ret))
        goto fail;
    ret = push_ptrs(&sp, stack_base, &envp_ptrs);
    if (IS_ERR(ret))
        goto fail;
    uintptr_t user_envp = sp;
    ptr_list_destroy(&envp_ptrs);

    ret = push_value(&sp, stack_base, 0);
    if (IS_ERR(ret))
        goto fail;
    ret = push_ptrs(&sp, stack_base, &argv_ptrs);
    if (IS_ERR(ret))
        goto fail;
    uintptr_t user_argv = sp;
    ptr_list_destroy(&argv_ptrs);

    sp = round_down(sp, 16);

    ret = push_value(&sp, stack_base, user_envp);
    if (IS_ERR(ret))
        goto fail;
    ret = push_value(&sp, stack_base, user_argv);
    if (IS_ERR(ret))
        goto fail;
    ret = push_value(&sp, stack_base, argc);
    if (IS_ERR(ret))
        goto fail;
    ret = push_value(&sp, stack_base, 0); // fake return address
    if (IS_ERR(ret))
        goto fail;

    paging_switch_page_directory(prev_pd);
    paging_destroy_current_page_directory();
    paging_switch_page_directory(new_pd);

    cli();

    current->vaddr_allocator = vaddr_allocator;
    current->eip = entry_point;
    current->esp = current->ebp = current->stack_top;
    current->ebx = current->esi = current->edi = 0;
    current->fpu_state = initial_fpu_state;

    strlcpy(current->comm, comm, sizeof(current->comm));

    // enter userland
    __asm__ volatile("movw $0x23, %%ax\n"
                     "movw %%ax, %%ds\n"
                     "movw %%ax, %%es\n"
                     "movw %%ax, %%fs\n"
                     "movw %%ax, %%gs\n"
                     "movl %0, %%esp\n"
                     "pushl $0x23\n"
                     "pushl %0\n"
                     "pushf\n"
                     "popl %%eax\n"
                     "orl $0x200, %%eax\n" // set IF
                     "pushl %%eax\n"
                     "pushl $0x1b\n"
                     "push %1\n"
                     "iret" ::"r"(sp),
                     "r"(entry_point)
                     : "eax");
    UNREACHABLE();

fail:
    ASSERT(IS_ERR(ret));

    kfree(executable_buf);
    string_list_destroy(&copied_envp);
    string_list_destroy(&copied_argv);
    ptr_list_destroy(&envp_ptrs);
    ptr_list_destroy(&argv_ptrs);

    paging_destroy_current_page_directory();
    paging_switch_page_directory(prev_pd);

    return ret;
    #endif
}
