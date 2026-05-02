#include "Syscalls.h"

#include "Exit/SyscallExit.h"
#include "Interrupts/Syscall.h"
#include "MMAP/SyscallMMAP.h"
#include "Read/SyscallRead.h"
#include "Yield/SyscallYield.h"
#include "Write/SyscallWrite.h"

namespace Syscalls {
    uint64_t SyscallNotImplemented(void *arg1, void *arg2, void *arg3, void *arg4, void *arg5, void *arg6, Interrupts::Syscall::SyscallFrame* frame) {
        LOG_WARNING("Usage of unimplemented syscall with args: %p, %p, %p, %p, %p, %p, %p", arg1, arg2, arg3, arg4, arg5, arg6, frame);
        return -1;
    }

    SyscallFunction SyscallTable[Syscalls::MAX_SYSCALLS] = {SyscallNotImplemented};

    void SyscallInitialize() {
        SyscallTable[(uint64_t)ID::EXIT] = Exit;
        SyscallTable[(uint64_t)ID::YIELD] = Yield;
        SyscallTable[(uint64_t)ID::MMAP] = MMAP;
        SyscallTable[(uint64_t)ID::WRITE] = Write;
        SyscallTable[(uint64_t)ID::READ] = Read;
    }

    SyscallFunction GetSyscallFunction(uint64_t syscallNumber) {
        if (syscallNumber >= MAX_SYSCALLS) {
            return SyscallNotImplemented;
        }

        return SyscallTable[syscallNumber];
    }
}
