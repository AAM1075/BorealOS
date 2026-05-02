#include "SyscallYield.h"
#include <Kernel.h>
#include "../../KernelData.h"
#include "../../Interrupts/IDT.h"

namespace Syscalls {
    SYSCALL_DEFINE(Yield) {
        asm volatile ("cli");

        Interrupts::IDT::Registers fakeRegisters = {
            .rax = frame->rax,
            .rbx = frame->rbx,
            .rcx = 0,
            .rdx = frame->rdx,
            .rsi = frame->rsi,
            .rdi = frame->rdi,
            .rbp = frame->rbp,
            .r8 = frame->r8,
            .r9 = frame->r9,
            .r10 = frame->r10,
            .r11 = frame->user_rflags,
            .r12 = frame->r12,
            .r13 = frame->r13,
            .r14 = frame->r14,
            .r15 = frame->r15,
            .error_code = 0,
            .rip = frame->user_rip,
            .cs = 0,
            .rflags = frame->user_rflags,
            .user_rsp = frame->user_rsp,
            .ss = 0
        };

        Kernel<KernelData>::GetInstance()->ArchitectureData->ThreadScheduler->Tick(&fakeRegisters);
        return -1; // Should not actually return, but just in case, we return -1 to indicate an error if it does.
    }
} // Syscalls