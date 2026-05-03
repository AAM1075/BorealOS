#ifndef BOREALOS_SYSCALLS_H
#define BOREALOS_SYSCALLS_H

#include <Definitions.h>
#include "Interrupts/Syscall.h"


#define SYSCALL_DEFINE(name) uint64_t name(void* arg1, void* arg2, void* arg3, void* arg4, void* arg5, void* arg6, Interrupts::Syscall::SyscallFrame* frame)
#define GET_ARG(T, n) static_cast<T>((uint64_t)arg##n)

// This is the main header for all other syscalls, include this to get the table that references all syscalls.
namespace Syscalls {
    enum class ID : uint64_t {
        EXIT = 60,
        YIELD = 24,
        MMAP = 9,
        WRITE = 1,
        READ = 0,
    };

    typedef uint64_t (*SyscallFunction)(void* arg1, void* arg2, void* arg3, void* arg4, void* arg5, void* arg6, Interrupts::Syscall::SyscallFrame* frame);
    constexpr uint32_t MAX_SYSCALLS = 256;

    void Initialize();

    uint64_t SyscallNotImplemented(void* arg1, void* arg2, void* arg3, void* arg4, void* arg5, void* arg6, Interrupts::Syscall::SyscallFrame* frame);
    extern SyscallFunction SyscallTable[MAX_SYSCALLS];
    SyscallFunction GetSyscallFunction(uint64_t syscallNumber);
}

#endif //BOREALOS_SYSCALLS_H
