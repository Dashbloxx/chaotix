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

#if defined(__i386__) && !defined(__x86_64__)

#include "boot_defs.h"
#define ASM_FILE
#include "multiboot.h"

#define FLAGS (MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO | MULTIBOOT_VIDEO_MODE)
#define CHECKSUM -(MULTIBOOT_HEADER_MAGIC + FLAGS)

  .section .multiboot, "a"
  .align MULTIBOOT_HEADER_ALIGN
  .long MULTIBOOT_HEADER_MAGIC
  .long FLAGS
  .long CHECKSUM

  // MULTIBOOT_AOUT_KLUDGE
  .long 0 // header_addr
  .long 0 // load_addr
  .long 0 // load_end_addr
  .long 0 // bss_end_addr
  .long 0 // entry_addr

  // MULTIBOOT_VIDEO_MODE
  .long 0  // mode_type
  .long 0  // width
  .long 0  // height
  .long 32 // depth

  .section .init_text, "a"
  .globl _start
_start:
  cli

  # fill page table
  movl $0x103, %esi # P | RW | G
  movl $kernel_page_table, %edi
  movl $1022, %ecx # last two pages are for quickmap
1:
  movl %esi, (%edi)
  addl $PAGE_SIZE, %esi
  addl $4, %edi
  loop 1b

  movl $kernel_page_table, %edx
  orl $3, %edx # P | RW
  movl %edx, kernel_page_directory # identity mapping
  movl %edx, kernel_page_directory + KERNEL_PDE_IDX * 4

  # recursive
  movl $kernel_page_directory, %edx
  orl $3, %edx # P | RW
  movl %edx, kernel_page_directory + 1023 * 4

  movl $kernel_page_directory, %edx
  movl %edx, %cr3

  # set PG | WP
  movl %cr0, %edx
  orl $0x80010000, %edx
  movl %edx, %cr0

  lea paging_enabled, %edx
  jmp *%edx

  .section .init_bss, "aw", @nobits
  .globl kernel_page_directory
kernel_page_directory:
  .align PAGE_SIZE
  .skip PAGE_SIZE
kernel_page_table:
  .align PAGE_SIZE
  .skip PAGE_SIZE

  .text
paging_enabled:
  # remove identity mapping
  movl $0, kernel_page_directory + KERNEL_VADDR
  movl $kernel_page_directory, %edx
  movl %edx, %cr3

  movl %cr4, %edx
  orl $0x80, %edx # PGE
  movl %edx, %cr4

  movl $stack_top, %esp
  pushl %ebx # Multiboot info struct
  pushl %eax # Multiboot magic

  movl $0x277, %ecx # IA32_PAT
  rdmsr
  andl $~0x7, %edx
  orl $1, %edx # PAT4 = WC
  wrmsr

  call start

  cli
loop_forever:
  hlt
  jmp loop_forever

  .globl read_eip
read_eip:
  popl %eax
  jmp *%eax

  .section .stack, "aw", @nobits
  .skip STACK_SIZE
  .globl stack_top
stack_top:
#elif defined(__x86_64__)
#include "boot_defs.h"
#define ASM_FILE
#include "multiboot.h"

#define FLAGS (MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO | MULTIBOOT_VIDEO_MODE)
#define CHECKSUM -(MULTIBOOT_HEADER_MAGIC + FLAGS)

  .section .multiboot, "a"
  .align MULTIBOOT_HEADER_ALIGN
  .long MULTIBOOT_HEADER_MAGIC
  .long FLAGS
  .long CHECKSUM

  // MULTIBOOT_AOUT_KLUDGE
  .long 0 // header_addr
  .long 0 // load_addr
  .long 0 // load_end_addr
  .long 0 // bss_end_addr
  .long 0 // entry_addr

  // MULTIBOOT_VIDEO_MODE
  .long 0  // mode_type
  .long 0  // width
  .long 0  // height
  .long 32 // depth

  .section .init_text, "a"
  .globl _start
_start:
  cli

  # fill page table
  movl $0x103, %esi # P | RW | G
  movl $kernel_page_table, %edi
  movl $1022, %ecx # last two pages are for quickmap
1:
  movl %esi, (%edi)
  addl $PAGE_SIZE, %esi
  addl $4, %edi
  loop 1b

  movl $kernel_page_table, %edx
  orl $3, %edx # P | RW
  movl %edx, kernel_page_directory # identity mapping
  movl %edx, kernel_page_directory + KERNEL_PDE_IDX * 4

  # recursive
  movl $kernel_page_directory, %edx
  orl $3, %edx # P | RW
  movl %edx, kernel_page_directory + 1023 * 4

  # setup page tables for long mode
  movl $kernel_page_directory, %eax
  movl %eax, %cr3

  movl %cr4, %eax
  orl $(1 << 5), %eax   # Enable PAE
  orl $(1 << 8), %eax   # Enable global pages
  movl %eax, %cr4

  movl %cr0, %eax
  orl $(1 << 31), %eax  # Enable paging
  movl %eax, %cr0

  # enter long mode
  movl $0xC0000080, %ecx
  rdmsr
  orl $(1 << 8), %eax   # Enable long mode
  wrmsr

  # set up long mode entry point
  movl $start_longmode, %eax
  movl %eax, 8(%esp)

  # jump to long mode entry point
  ljmp $0x8, $start_longmode

start_longmode:
  # clear segment registers
  xorl %eax, %eax
  movl %eax, %ds
  movl %eax, %es
  movl %eax, %fs
  movl %eax, %gs

  # set up kernel stack
  movl $stack_top, %esp

  # set up kernel data segment
  movl $kernel_ds, %eax
  movl %eax, %ds
  movl %eax, %es
  movl %eax, %fs
  movl %eax, %gs

  # clear BSS
  cld
  movl $0, %edi
  movl $end, %ecx
  subl $end, %edi
  xorl %eax, %eax
  rep stosb

  # call main function
  call main

  # halt
  cli
loop_forever:
  hlt
  jmp loop_forever

  # read EIP function
  .globl read_eip
read_eip:
  popl %eax
  jmp *%eax

  # stack section
  .section .stack, "aw", @nobits
  .skip STACK_SIZE
  .globl stack_top
stack_top:

  # data section
  .section .data
end:

#elif defined(__aarch64__) && defined(__RPi__)
// AArch64 mode
 
// To keep this in the first portion of the binary.
.section ".text.boot"
 
// Make _start global.
.globl _start
 
    .org 0x80000
// Entry point for the kernel. Registers:
// x0 -> 32 bit pointer to DTB in memory (primary core only) / 0 (secondary cores)
// x1 -> 0
// x2 -> 0
// x3 -> 0
// x4 -> 32 bit kernel entry point, _start location
_start:
    // set stack before our code
    ldr     x5, =_start
    mov     sp, x5
 
    // clear bss
    ldr     x5, =__bss_start
    ldr     w6, =__bss_size
1:  cbz     w6, 2f
    str     xzr, [x5], #8
    sub     w6, w6, #1
    cbnz    w6, 1b
 
    // jump to C code, should not return
2:  bl      start
    // for failsafe, halt this core
halt:
    wfe
    b halt
#endif