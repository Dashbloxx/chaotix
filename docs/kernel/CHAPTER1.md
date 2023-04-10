# Chapter 1
## Booting up
The bootloader loads the kernel, and passes some useful information that the kernel can't get on its own (the bootloader has to get it). The kernel uses the multiboot protocol to get the information from the bootloader. There are several bootloaders that support multiboot, like the (GRUB)[https://www.gnu.org/software/grub/] bootloader, (limine)[https://limine-bootloader.org/] bootloader, and several other bootloaders. The two bootloaders I just provided work with multiboot, therefore they should cooperate with Chaotix's kernel.
Now, the bootloader jumps to the code in `boot.S`, an assembly file that obtains multiboot and then passes it to the `start` function in `main.c`. The `start` function then does it's thing:
1. Enables GDT (Global Descriptor Table), a i?86/AMD64-specific thing.
2. Enables IDT (Interrupt Descriptor Table), which is also a i?86/AMD64-specific thing.
3. Enables the IRQ (Interrupt Request) driver, which is ALSO a i?86/AMD64-specific thing.
4. Enables serial driver.
5. Enables basic I/O like terminal functionality, so that logging works.
6. Enables paging if running on i?86/AMD64.
7. Initialize kernel process.
8. Initialize the filesystem. In more detailed explanation, the `/tmp`, `/dev`, `/dev/shm`, and `/proc` directories are created, and then the character devices are created into `/dev`, like the virtual character devices, the framebuffer character device, the teletype character device, keyboard, mouse, and sound character devices, and then the serial teletype character devices, aswell as the console character device.
9. Initialize syscalls
10. Initialize schelduer
11. Initialize timer
12. Initialize PIC (i?86/AMD64-only)
13. Start a process as user-mode called `init` (can be found at `userland/init.c`)