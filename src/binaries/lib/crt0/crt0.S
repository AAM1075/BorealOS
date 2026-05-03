.extern main

.global _start
.type _start,@function
_start:
    xor %rbp, %rbp
    pop %rdi // argc
    mov %rsp, %rsi // argv
    call main
    mov %rax, %rdi
    mov $60, %rax // sys_exit
    syscall
