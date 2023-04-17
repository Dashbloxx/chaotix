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

#include "../common/escp.h"
#include "api/sys/stat.h"
#include "boot_defs.h"
#include "console/console.h"
#include "graphics/graphics.h"
#include "hid/hid.h"
#include "interrupts.h"
#include "kprintf.h"
#include "random.h"
#include "memory/memory.h"
#include "multiboot.h"
#include "panic.h"
#include "process.h"
#include "scheduler.h"
#include "serial.h"
#include "syscall/syscall.h"
#include "system.h"

/*
 *  Call a userland binary called `init`, which can be found in the `bin` directory. 
 */
static noreturn void init(void) {
    current->pid = current->pgid = process_generate_next_pid();

    const char* init_path = cmdline_lookup("init");
    if (!init_path)
        init_path = "/bin/init";
    const char* argv[] = {init_path, NULL};
    static const char* envp[] = {NULL};
    ASSERT_OK(sys_execve(init_path, (char* const*)argv, (char* const*)envp));
    UNREACHABLE();
}

/*
 *  These two objects are very useful for knowing the boundaries of the
 *  kernel in memory.
 */
extern unsigned char kernel_end[];
extern unsigned char stack_top[];

/*
 *  Create a character device (a file which points to a device or module in the kernel).
 */
static void create_char_device(const char* pathname, struct inode* device) {
    ASSERT_OK(sys_mknod(pathname, S_IFCHR, device->device_id));
    ASSERT_OK(vfs_register_device(device));
}

/*
 *  This function is called from assembly. Notice that this function also contains two arguments. These arguments are related to multiboot. Multiboot allows bootloaders
 *  to collect data (which the kernel can't collect), and then pass it to the kernel when jumping to it.
 */
#if defined(__aarch64__) && defined(__RPi__)
void start(uint64_t dtb_ptr32, uint64_t x1, uint64_t x2, uint64_t x3) {
#elif defined(__aarch32__) && defined(__RPi__)
void start(uint32_t r0, uint32_t r1, uint32_t atags) {
#elif defined(__i386__)
void start(uint32_t mb_magic, uintptr_t mb_info_paddr) {
#endif
    /*
     *  We are only using cross-compilers to build the Chaotix operating system. If we use a x86-compatible cross-compiler, it will by default build us Chaotix for
     *  the architecture that the cross-compiler compiles for. For now though, i?86 is supported, meaning that the i686-elf cross-toolchain works for building Chaotix.
     *  When i?86 is stated, we mean i386, i486, i586, i686, or i786.
     *  Anyways, here, we run some i?86-specific stuff...
     */
    #if defined(__i386__)
    /*
     *  Initialize the GDT (Global Descriptor Table). This is a table that can be defined as a struct in C. This struct's pointer can be given to the CPU so that it
     *  knows the characteristics (and the segments themselves) of each segment in memory.
     */
    gdt_init();

    /*
     *  Initialize the IDT (Interrupt Descriptor Table). This will be documented later.
     */
    idt_init();

    /*
     *  Initialize the IRQ (Interrupt Request) driver. This will be documented later.
     */
    irq_init();
    #endif

    /*
     *  Initialize the serial driver.
     */
    serial_init();

    kprintf("%s%s[%s+%s] %s%sBooted%s\n", BOLD, F_CYAN, F_BLUE, F_CYAN, RESET, F_GREEN, RESET);

    /* The STI function is i?86-specific! */
    #if defined(__i386__)
    sti();
    #endif

    /* Print out kernel's boundaries... */
    ASSERT(mb_magic == MULTIBOOT_BOOTLOADER_MAGIC);
    kprintf("Kernel stack top: V0x%x\n", (uintptr_t)stack_top);
    kprintf("Kernel end: V0x%x\n", (uintptr_t)kernel_end);

    const multiboot_info_t* mb_info = (const multiboot_info_t*)(mb_info_paddr + KERNEL_VADDR);

    cmdline_init(mb_info);

    const multiboot_module_t* initrd_mod = (const multiboot_module_t*)(mb_info->mods_addr + KERNEL_VADDR);
    uintptr_t initrd_paddr = initrd_mod->mod_start;
    uintptr_t initrd_size = initrd_mod->mod_end - initrd_mod->mod_start;

    /*
     *  Although paging isn't a i?86-specific thing, we are keeping this
     *  mechanism for i?86 only for now since setting up paging in other
     *  architectures isn't the top priority now...
     */
    #if defined(__i386__)
    /*
     *  Initialize paging. Paging is a mechanism found in the x86 & x86_64 architecture (x86 & x86_64 specific) which allows to be more memory efficient.
     */
    paging_init(mb_info);
    #endif

    ASSERT_OK(vfs_mount(ROOT_DIR, tmpfs_create_root()));

    process_init();

    initrd_populate_root_fs(initrd_paddr, initrd_size);

    ASSERT_OK(vfs_mount("/tmp", tmpfs_create_root()));
    ASSERT_OK(vfs_mount("/dev/shm", tmpfs_create_root()));
    ASSERT_OK(vfs_mount("/proc", procfs_create_root()));

    create_char_device("/dev/null", null_device_create());
    create_char_device("/dev/zero", zero_device_create());
    create_char_device("/dev/full", full_device_create());
    create_char_device("/dev/random", random_device_create());

    /*
     *  Initialize the framebuffer using the information given by the multiboot header. If the framebuffer initialization is successful, then create a character device
     *  for the framebuffer, and initialize the framebuffer console. After it is initialized, create a virtual TTY character device for accessing the framebuffer
     *  console.
     *  Note that in the future the framebuffer console may be replaced with a natural console (VGA text mode), so that it can be possible to display a window server
     *  without wasting resources because of the framebuffer console.
     *  See `kernel/console/fb_console.c` for the code regarding the framebuffer console.
     */
    if (fb_init(mb_info)) {
        create_char_device("/dev/fb0", fb_device_create());
        fb_console_init();
        create_char_device("/dev/tty", fb_console_device_create());
    }

    /*
     *  Initialize the PS/2 driver. Then, create a character device for the PS/2 keyboard and PS/2 mouse.
     *  Because PS/2 is commonly a i?86-only thing, we limit it to only i?86 when compiling to it.
     */
    #if defined(__i386__)
    ps2_init();
    create_char_device("/dev/kbd", ps2_keyboard_device_create());
    create_char_device("/dev/psaux", ps2_mouse_device_create());
    #endif

    /*
     *  AC97 isn't really common on other architectures, therefore we just make this a i?86-only thing.
     */
    #if defined(__i386__)
    /*
     *  Initialize the AC97 driver. If the device exists, then create a character device named `dsp`.
     */
    if (ac97_init())
        create_char_device("/dev/dsp", ac97_device_create());
    #endif

    /*
     *  Initialize the serial console, and serial ports COM1-COM3 as character devices. These character devices are known as `ttyS?`. The serial driver however, is
     *  initialized way earlier.
     */
    serial_console_init();
    if (serial_enable_port(SERIAL_COM1))
        create_char_device("/dev/ttyS0", serial_console_device_create(SERIAL_COM1));
    if (serial_enable_port(SERIAL_COM2))
        create_char_device("/dev/ttyS1", serial_console_device_create(SERIAL_COM2));
    if (serial_enable_port(SERIAL_COM3))
        create_char_device("/dev/ttyS2", serial_console_device_create(SERIAL_COM3));
    if (serial_enable_port(SERIAL_COM4))
        create_char_device("/dev/ttyS3", serial_console_device_create(SERIAL_COM4));

    /*
     *  Initialize the console character device.
     */
    system_console_init();
    create_char_device("/dev/console", system_console_device_create());

    syscall_init();
    scheduler_init();
    time_init();
    
    /* 
     *  PIT is i?86-specific, so we make this work only when compiling
     *  for an i?86 platform...
     */
    #if defined(__i386__)
    pit_init();
    #endif
    
    kprintf("%s%s[%s+%s] %s%sInitialization done%s\n", BOLD, F_CYAN, F_BLUE, F_CYAN, RESET, F_GREEN, RESET);

    /* Start a process called `init`, which spawns shells on all TTYs in user mode... */
    ASSERT_OK(process_spawn_kernel_process("userland_init", init));

    process_exit(0);
}
