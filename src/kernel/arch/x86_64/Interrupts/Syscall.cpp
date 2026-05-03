#include "Syscall.h"

#include "Kernel.h"
#include "KernelData.h"
#include "TSS.h"
#include "Syscalls/Syscalls.h"

#define MSR_EFER 0xC0000080
#define MSR_STAR 0xC0000081
#define MSR_LSTAR 0xC0000082
#define MSR_FMASK 0xC0000084

extern "C" {
    extern void SyscallHandler();
    [[noreturn]] extern void EnterUserspace(uint64_t entryPoint, uint64_t userStack);
    [[noreturn]] extern void ResumeThread(Interrupts::IDT::Registers* registers);

    uint64_t KernelSyscallHandler(Interrupts::Syscall::SyscallFrame* frame) {
        auto syscallFunction = Syscalls::GetSyscallFunction(frame->rax);
        return syscallFunction((void*)frame->rdi, (void*)frame->rsi, (void*)frame->rdx, (void*)frame->r10, (void*)frame->r8, (void*)frame->r9, frame);
    }

    uint64_t syscall_kernel_rsp;
}

namespace Interrupts {
    void Syscall::Initialize() {
        // SCE bit (system call extensions) in EFER MSR
        uint64_t efer = Core::CPU::ReadMSR(MSR_EFER);
        efer |= 1; // Set SCE bit
        Core::CPU::WriteMSR(MSR_EFER, efer);

        // Set STAR MSR to define syscall/sysret segments and CPL3 code segment selector
        Core::CPU::WriteMSR(MSR_STAR, ((uint64_t)0x28 << 48) | ((uint64_t)0x08 << 32));
        Core::CPU::WriteMSR(MSR_LSTAR, (uint64_t)&SyscallHandler); // called when syscall is invoked with syscall instruction
        Core::CPU::WriteMSR(MSR_FMASK, 1 << 9);
    }

    void Syscall::EnterUserspace(uint64_t entryPoint, uint64_t userStack) {
        auto tss = TSS::GetTSSStruct();
        syscall_kernel_rsp = tss->RSP0; // Save the current kernel RSP so we can restore it when we return to the kernel.

        ::EnterUserspace(entryPoint, userStack);
    }

    void Syscall::ResumeThread(IDT::Registers *registers) {
        ::ResumeThread(registers);
    }
} // Interrupts