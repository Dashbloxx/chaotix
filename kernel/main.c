#include "../common/escp.h"
#include "api/sys/stat.h"
#include "boot_defs.h"
#include "console/console.h"
#include "graphics/graphics.h"
#include "hid/hid.h"
#include "interrupts.h"
#include "kprintf.h"
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
void start(uint32_t mb_magic, uintptr_t mb_info_paddr) {
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

    /*
     *  Initialize the serial driver.
     */
    serial_init();

    kprintf("\x1b[32mBooted\x1b[m\n");
    sti();

    ASSERT(mb_magic == MULTIBOOT_BOOTLOADER_MAGIC);
    kprintf("Kernel stack top: V0x%x\n", (uintptr_t)stack_top);
    kprintf("Kernel end: V0x%x\n", (uintptr_t)kernel_end);

    const multiboot_info_t* mb_info = (const multiboot_info_t*)(mb_info_paddr + KERNEL_VADDR);

    cmdline_init(mb_info);

    const multiboot_module_t* initrd_mod = (const multiboot_module_t*)(mb_info->mods_addr + KERNEL_VADDR);
    uintptr_t initrd_paddr = initrd_mod->mod_start;
    uintptr_t initrd_size = initrd_mod->mod_end - initrd_mod->mod_start;

    /*
     *  Initialize paging. Paging is a mechanism found in the x86 & x86_64 architecture (x86 & x86_64 specific) which allows to be more memory efficient.
     */
    paging_init(mb_info);

    ASSERT_OK(vfs_mount(ROOT_DIR, tmpfs_create_root()));

    process_init();

    initrd_populate_root_fs(initrd_paddr, initrd_size);

    ASSERT_OK(vfs_mount("/tmp", tmpfs_create_root()));
    ASSERT_OK(vfs_mount("/dev/shm", tmpfs_create_root()));
    ASSERT_OK(vfs_mount("/proc", procfs_create_root()));

    create_char_device("/dev/null", null_device_create());
    create_char_device("/dev/zero", zero_device_create());
    create_char_device("/dev/full", full_device_create());

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
     */
    ps2_init();
    create_char_device("/dev/kbd", ps2_keyboard_device_create());
    create_char_device("/dev/psaux", ps2_mouse_device_create());

    /*
     *  Initialize the AC97 driver. If the device exists, then create a character device named `dsp`.
     */
    if (ac97_init())
        create_char_device("/dev/dsp", ac97_device_create());

    /*
     *  Initialize the serial console, and serial ports COM1-COM3 as character devices. These character devices are known as `ttyS?`. The serial driver however, is
     *  initialized way earlier.
     */
    serial_console_init();
    if (serial_enable_port(SERIAL_COM1))
        create_char_device("/dev/ttyS0", serial_console_device_create(SERIAL_COM1));
    if (serial_enable_port(SERIAL_COM2))
        create_char_device("/dev/ttyS1", serial_console_device_create(SERIAL_COM2));
    if (serial_enable_port(SERIAL_COM2))
        create_char_device("/dev/ttyS2", serial_console_device_create(SERIAL_COM3));
    if (serial_enable_port(SERIAL_COM3))
        create_char_device("/dev/ttyS3", serial_console_device_create(SERIAL_COM4));

    /*
     *  Initialize the console character device.
     */
    system_console_init();
    create_char_device("/dev/console", system_console_device_create());

    syscall_init();
    scheduler_init();
    time_init();
    pit_init();
    kprintf(F_GREEN "Initialization done\x1b[m\n");

    ASSERT_OK(process_spawn_kernel_process("userland_init", init));

    process_exit(0);
}
