#include "memory.h"
#include <common/extra.h>
#include <kernel/api/sys/types.h>
#include <kernel/boot_defs.h>
#include <kernel/kprintf.h>
#include <kernel/lock.h>
#include <kernel/multiboot.h>
#include <kernel/panic.h>
#include <stdbool.h>

#define MAX_NUM_PAGES (1024 * 1024)
#define BITMAP_MAX_LEN (MAX_NUM_PAGES / 32)

static size_t bitmap_len;
static uint32_t bitmap[BITMAP_MAX_LEN];
static uint8_t ref_counts[MAX_NUM_PAGES];
static mutex lock;

static bool bitmap_get(size_t i) {
    ASSERT((i >> 5) < bitmap_len);
    return bitmap[i >> 5] & (1 << (i & 31));
}

static void bitmap_set(size_t i) {
    ASSERT((i >> 5) < bitmap_len);
    bitmap[i >> 5] |= 1 << (i & 31);
}

static void bitmap_clear(size_t i) {
    ASSERT((i >> 5) < bitmap_len);
    bitmap[i >> 5] &= ~(1 << (i & 31));
}

static ssize_t bitmap_find_first_set(void) {
    for (size_t i = 0; i < bitmap_len; ++i) {
        int b = __builtin_ffs(bitmap[i]);
        if (b > 0) // b == 0 if bitmap[i] == 0
            return (i << 5) | (b - 1);
    }
    return -ENOMEM;
}

extern unsigned char kernel_end[];

/* Get physical memory bounds using the multiboot headers... */
static void get_available_physical_addr_bounds(const multiboot_info_t* mb_info, uintptr_t* lower_bound, uintptr_t* upper_bound) {
    *lower_bound = (uintptr_t)kernel_end - KERNEL_VADDR;

    if (!(mb_info->flags & MULTIBOOT_INFO_MEM_MAP)) {
        *upper_bound = mb_info->mem_upper * 0x400 + 0x100000;
        return;
    }

    uint32_t num_entries = mb_info->mmap_length / sizeof(multiboot_memory_map_t);
    const multiboot_memory_map_t* entry = (const multiboot_memory_map_t*)(mb_info->mmap_addr + KERNEL_VADDR);

    *upper_bound = *lower_bound;
    for (uint32_t i = 0; i < num_entries; ++i, ++entry) {
        if (entry->type != MULTIBOOT_MEMORY_AVAILABLE)
            continue;

        uintptr_t entry_end = entry->addr + entry->len;
        if (*upper_bound < entry_end)
            *upper_bound = entry_end;
    }
}

static struct physical_memory_info memory_info;

static void bitmap_init(const multiboot_info_t* mb_info, uintptr_t lower_bound, uintptr_t upper_bound) {
    bitmap_len = div_ceil(upper_bound, PAGE_SIZE * 32);
    ASSERT(bitmap_len <= BITMAP_MAX_LEN);

    if (mb_info->flags & MULTIBOOT_INFO_MEM_MAP) {
        uint32_t num_entries = mb_info->mmap_length / sizeof(multiboot_memory_map_t);
        const multiboot_memory_map_t* entry = (const multiboot_memory_map_t*)(mb_info->mmap_addr + KERNEL_VADDR);

        for (uint32_t i = 0; i < num_entries; ++i, ++entry) {
            if (entry->type != MULTIBOOT_MEMORY_AVAILABLE)
                continue;

            uintptr_t entry_start = entry->addr;
            uintptr_t entry_end = entry->addr + entry->len;

            kprintf("Available region: P0x%08x - P0x%08x (%u MiB)\n", entry_start, entry_end, (entry_end - entry_start) / 0x100000);

            if (entry_start < lower_bound)
                entry_start = lower_bound;

            if (entry_start >= entry_end)
                continue;

            for (size_t i = div_ceil(entry_start, PAGE_SIZE); i < entry_end / PAGE_SIZE; ++i)
                bitmap_set(i);
        }
    } else {
        for (size_t i = div_ceil(lower_bound, PAGE_SIZE); i < upper_bound / PAGE_SIZE; ++i)
            bitmap_set(i);
    }

    if (mb_info->flags & MULTIBOOT_INFO_MODS) {
        const multiboot_module_t* mod = (const multiboot_module_t*)(mb_info->mods_addr + KERNEL_VADDR);
        for (uint32_t i = 0; i < mb_info->mods_count; ++i) {
            kprintf("Module: P0x%08x - P0x%08x (%u MiB)\n", mod->mod_start, mod->mod_end, (mod->mod_end - mod->mod_start) / 0x100000);
            for (size_t i = mod->mod_start / PAGE_SIZE; i < div_ceil(mod->mod_end, PAGE_SIZE); ++i)
                bitmap_clear(i);
            ++mod;
        }
    }

    size_t num_pages = 0;
    for (size_t i = 0; i < bitmap_len * 32; ++i) {
        if (bitmap_get(i)) {
            ++num_pages;
        } else {
            // By setting initial reference counts to be non-zero values,
            // the reference counts of these pages will never reach zero,
            // avoiding accidentaly marking the pages available for allocation.
            ref_counts[i] = UINT8_MAX;
        }
    }
    memory_info.total = memory_info.free = num_pages * PAGE_SIZE / 1024;
    kprintf("#Physical pages: %u (%u KiB)\n", num_pages, memory_info.total);
}

/*
 *  Initialize the page allocator using the multiboot header. The multiboot header contains values given by the bootloader. Only the bootloader can give us these
 *  values (because you can only get these values while in real mode, and the bootloader is in real mode at the start).
 */
void page_allocator_init(const multiboot_info_t* mb_info) {
    // In the current setup, kernel image (including 1MiB offset) has to fit in
    // single page table (< 4MiB), and last two pages are reserved for quickmap
    ASSERT((uintptr_t)kernel_end <= KERNEL_VADDR + 1022 * PAGE_SIZE);

    uintptr_t lower_bound;
    uintptr_t upper_bound;
    get_available_physical_addr_bounds(mb_info, &lower_bound, &upper_bound);
    kprintf("Available physical memory address space: P0x%x - P0x%x\n", lower_bound, upper_bound);

    bitmap_init(mb_info, lower_bound, upper_bound);
}

uintptr_t page_allocator_alloc(void) {
    mutex_lock(&lock);

    ssize_t first_set = bitmap_find_first_set();
    if (IS_ERR(first_set)) {
        mutex_unlock(&lock);
        kputs("Out of physical pages\n");
        return first_set;
    }

    bitmap_clear(first_set);
    ASSERT(ref_counts[first_set] == 0);
    ref_counts[first_set] = 1;
    memory_info.free -= PAGE_SIZE / 1024;

    mutex_unlock(&lock);
    return first_set * PAGE_SIZE;
}

void page_allocator_ref_page(uintptr_t physical_addr) {
    ASSERT(physical_addr % PAGE_SIZE == 0);
    size_t idx = physical_addr / PAGE_SIZE;

    mutex_lock(&lock);

    if (ref_counts[idx] < UINT8_MAX)
        ++ref_counts[idx];

    mutex_unlock(&lock);
}

void page_allocator_unref_page(uintptr_t physical_addr) {
    ASSERT(physical_addr % PAGE_SIZE == 0);
    size_t idx = physical_addr / PAGE_SIZE;

    mutex_lock(&lock);

    ASSERT(ref_counts[idx] > 0);

    // When the reference count is UINT8_MAX, we can't tell whether it actually
    // has exactly UINT8_MAX references or the count was saturated.
    // To be safe, we never decrement the reference count if count == UINT8_MAX
    // assuming the count was saturated.
    if (ref_counts[idx] < UINT8_MAX) {
        if (--ref_counts[idx] == 0) {
            bitmap_set(idx);
            memory_info.free += PAGE_SIZE / 1024;
        }
    }

    mutex_unlock(&lock);
}

void page_allocator_get_info(struct physical_memory_info* out_memory_info) {
    mutex_lock(&lock);
    *out_memory_info = memory_info;
    mutex_unlock(&lock);
}
