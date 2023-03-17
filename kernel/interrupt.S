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
