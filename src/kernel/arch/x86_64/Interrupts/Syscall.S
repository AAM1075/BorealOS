.extern syscall_kernel_rsp
.extern KernelSyscallHandler

.section .data
user_rsp_scratch:
    .quad 0

.section .text
.global SyscallHandler
.type SyscallHandler,@function
SyscallHandler:
    // Save user RSP and switch to kernel stack
    mov %rsp, user_rsp_scratch(%rip)
    mov syscall_kernel_rsp(%rip), %rsp

    // Construct frame: RIP, RFLAGS, RSP, then GPRs
    push %rcx                   // User RIP
    push %r11                   // User RFLAGS
    push user_rsp_scratch(%rip) // User RSP
    push %rax
    push %rdi
    push %rsi
    push %rdx
    push %r10
    push %r8
    push %r9
    push %rbx
    push %rbp
    push %r12
    push %r13
    push %r14
    push %r15

    mov %rsp, %rdi
    call KernelSyscallHandler

    // Restore GPRs
    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %rbp
    pop %rbx
    pop %r9
    pop %r8
    pop %r10
    pop %rdx
    pop %rsi
    pop %rdi
    add $8, %rsp // Skip %rax, so we keep the syscall return value intact

    // Restore context without switching RSP prematurely
    pop %rdx    // Pop User RSP into a scratch register (RDX)
    pop %r11    // Pop User RFLAGS into R11 (required for sysretq)
    pop %rcx    // Pop User RIP into RCX (required for sysretq)

    // Final switch to user stack and return
    mov %rdx, %rsp
    sysretq

//     extern void EnterUserspace(uint64_t entryPoint, uint64_t userStack);
.global EnterUserspace
.type EnterUserspace,@function
EnterUserspace:
    push $0x33 // SS selector (GDT entry 3)
    push %rsi // User stack pointer
    pushfq
    pop %rax
    or $0x200, %rax // Set IF in RFLAGS to enable interrupts in userspace
    push %rax
    push $0x3B // CS selector (GDT entry 4)
    push %rdi // User instruction pointer
    mov $0, %rax
    sti // Enable interrupts, so we can schedule other stuff later.
    iretq

// extern void ResumeThread(Interrupts::IDT::Registers* registers);
.global ResumeThread
.type ResumeThread,@function
ResumeThread:
    // rdi = pointer to Registers struct
    // Build iretq frame first, then restore GPRs

    // Region: iretq frame
    mov $0x33, %rax    // User SS selector (GDT entry 4)
    push %rax
    mov 0x98(%rdi), %rax    // user_rsp
    push %rax
    mov 0x90(%rdi), %rax    // rflags
    push %rax
    mov $0x3B, %rax    // User CS selector (GDT entry 3)
    push %rax
    mov 0x80(%rdi), %rax    // rip
    push %rax

    mov 0x00(%rdi), %rax
    mov 0x08(%rdi), %rbx
    mov 0x18(%rdi), %rdx
    mov 0x30(%rdi), %rbp
    mov 0x38(%rdi), %r8
    mov 0x40(%rdi), %r9
    mov 0x40(%rdi), %r10
    mov 0x48(%rdi), %r11
    mov 0x58(%rdi), %r12
    mov 0x60(%rdi), %r13
    mov 0x68(%rdi), %r14
    mov 0x70(%rdi), %r15
    mov 0x20(%rdi), %rsi
    mov 0x28(%rdi), %rdi

    iretq