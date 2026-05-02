#include "SyscallExit.h"

#include "Kernel.h"
#include "../../KernelData.h"

namespace Syscalls {
    SYSCALL_DEFINE(Exit) {
        auto exitCode = GET_ARG(int, 1);
        auto ts = Kernel<KernelData>::GetInstance()->ArchitectureData->ThreadScheduler;
        auto currentThread = ts->GetCurrentThread();
        auto currentProcess = ts->GetCurrentProcess();

        LOG_INFO("Process %d exited with code %d.", currentProcess->pid, exitCode);
        ts->ExitThread(currentProcess, currentThread);
        return 0; // This will never actually be returned, since ExitThread should not return, but just in case, we return 0 to indicate success if it does.
    }
} // Syscalls