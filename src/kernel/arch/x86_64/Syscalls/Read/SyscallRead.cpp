#include "SyscallRead.h"

#include "Kernel.h"
#include "../../KernelData.h"

namespace Syscalls {
    SYSCALL_DEFINE(Read) {
        auto fd = GET_ARG(int, 1);
        auto buffer = (char*)GET_ARG(uintptr_t, 2);
        auto count = GET_ARG(size_t, 3);

        if (buffer == nullptr) {
            return -1; // Invalid pointer
        }

        if (IS_HIGHER_HALF(buffer)) { // Illegal access, user programs can not write to kernel memory
            return -1;
        }

        auto scheduler = Kernel<KernelData>::GetInstance()->ArchitectureData->ThreadScheduler;
        auto process = scheduler->GetCurrentProcess();
        if (fd < 0 || (size_t)fd >= process->openFiles.Size()) {
            return -1; // Invalid file descriptor
        }

        auto descriptor = process->openFiles[fd];
        if (!descriptor || !descriptor->fs || !descriptor->file) {
            return -1; // Invalid file descriptor
        }

        auto result = descriptor->fs->Read(descriptor->file, buffer, count);
        if (result == (size_t)-1) {
            return -1; // Write failed
        }

        return count;
    }
} // Syscalls