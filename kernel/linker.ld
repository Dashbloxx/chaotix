ENTRY (_start)

PHDRS {
  multiboot PT_LOAD;
  init_text PT_LOAD;
  init_bss PT_LOAD;
  text PT_LOAD;
  data PT_LOAD;
  bss PT_LOAD;
}

KERNEL_VADDR = 0xc0000000;

SECTIONS {
    . = 0x100000;

    .multiboot ALIGN(4K) : AT(ADDR(.multiboot)) {
        KEEP(*(.multiboot))
    } :multiboot

    .init_text : AT(ADDR(.init_text)) {
        *(.init_text)
    } :init_text

    .init_bss ALIGN(4K) : AT(ADDR(.init_bss)) {
        *(.init_bss)
    } :init_bss

    . += KERNEL_VADDR;

    .text ALIGN(4K) : AT(ADDR(.text) - KERNEL_VADDR) {
        *(.text)
    } :text

    .rodata ALIGN (4K) : AT(ADDR(.rodata) - KERNEL_VADDR) {
        *(.rodata)
    } :data

    .data ALIGN (4K) : AT(ADDR(.data) - KERNEL_VADDR) {
        *(.data)
    } :data

    .bss ALIGN(4K) : AT(ADDR(.bss) - KERNEL_VADDR) {
        *(COMMON)
        *(.bss)
        *(.stack)
    } :bss

    kernel_end = .;
}
