/*
 *    __  __                      
 *   |  \/  |__ _ __ _ _ __  __ _ 
 *   | |\/| / _` / _` | '  \/ _` |
 *   |_|  |_\__,_\__, |_|_|_\__,_|
 *               |___/        
 * 
 *  Magma is a UNIX-like operating system that consists of a kernel written in C and
 *  i?86 assembly, and userland binaries written in C.
 *     
 *  Copyright (c) 2023 Nexuss, John Paul Wohlscheid, rilysh, Milton612, and FueledByCocaine
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

#if defined(__i386__)

  .globl isr_common_stub
isr_common_stub:
  pusha
  pushl %ds
  pushl %es
  pushl %fs
  pushl %gs
  pushl %ss

  movw $0x10, %ax
  movw %ax, %ds
  movw %ax, %es
  movw %ax, %fs
  movw %ax, %gs

  movl %esp, %eax
  pushl %eax

  call isr_handler

  addl $4, %esp # pop esp
  jmp return_to_userland

  .globl irq_common_stub
irq_common_stub:
  pusha
  pushl %ds
  pushl %es
  pushl %fs
  pushl %gs
  pushl %ss

  movw $0x10, %ax
  movw %ax, %ds
  movw %ax, %es
  movw %ax, %fs
  movw %ax, %gs

  movl %esp, %eax
  pushl %eax

  call irq_handler

  addl $4, %esp # pop esp
  jmp return_to_userland

  .globl return_to_userland
return_to_userland:
  addl $4, %esp # pop ss
  popl %gs
  popl %fs
  popl %es
  popl %ds
  popa

  addl $8, %esp # pop err_code and num
  iret

#endif