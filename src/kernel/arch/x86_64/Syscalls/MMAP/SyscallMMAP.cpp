#include "SyscallMMAP.h"

#include "Kernel.h"
#include "../../KernelData.h"

namespace Syscalls {
    enum class MMapFlags : int {
        MAP_SHARED = 0x01,
        MAP_PRIVATE = 0x02,
        MAP_ANONYMOUS = 0x20,
    };

    enum class MMapProt : int {
        PROT_NONE = 0x0,
        PROT_READ = 0x1,
        PROT_WRITE = 0x2,
        PROT_EXEC = 0x4,
    };

    static constexpr uintptr_t MAP_FAILED = ~0ULL;
    static constexpr uintptr_t USER_MMAP_BASE = 0x0000100000000000ULL;
    static constexpr uintptr_t USER_MMAP_END  = 0x00007F0000000000ULL;

    static Memory::PageFlags ProtToPageFlags(MMapProt prot) {
        // User pages are always Present | User; kernel sets RW by default.
        // NoExecute is applied unless PROT_EXEC is set — NX requires EFER.NXE to be set.
        auto flags = Memory::PageFlags::Present | Memory::PageFlags::User;

        if (((int)prot & (int)MMapProt::PROT_WRITE) != 0)
            flags |= Memory::PageFlags::ReadWrite;

        if (((int)prot & (int)MMapProt::PROT_EXEC) == 0)
            flags |= Memory::PageFlags::NoExecute;

        return flags;
    }

    SYSCALL_DEFINE(MMAP) {
        auto addr = GET_ARG(uintptr_t, 1);
        auto length = GET_ARG(size_t, 2);
        auto prot = GET_ARG(MMapProt, 3);
        auto flags = GET_ARG(MMapFlags, 4);
        auto fd = GET_ARG(int, 5);
        auto offset = GET_ARG(size_t, 6);

        // For now, we will only support anonymous mappings, and ignore the fd and offset parameters.
        if (((int)flags & (int)MMapFlags::MAP_ANONYMOUS) == 0) {
            LOG_ERROR("MMAP syscall only supports anonymous mappings for now!");
            return MAP_FAILED;
        }

        if (length == 0) {
            return MAP_FAILED;
        }

        auto pageCount = (length + Architecture::KernelPageSize - 1) / Architecture::KernelPageSize;
        auto kernel = Kernel<KernelData>::GetInstance();
        auto paging = &kernel->ArchitectureData->Paging;

        auto region = paging->GetAvailableVirtualAddress(pageCount, USER_MMAP_END, USER_MMAP_BASE);
        if (region.start == 0) {
            LOG_ERROR("MMAP syscall failed to find available virtual address range!");
            return MAP_FAILED;
        }

        Memory::PageFlags pageFlags = ProtToPageFlags(prot);
        uintptr_t phys = kernel->ArchitectureData->Pmm.AllocatePages(pageCount);

        if (!phys) {
            LOG_ERROR("MMAP syscall failed to allocate physical memory!");
            return MAP_FAILED;
        }

        paging->MapPages(region.start, phys, pageCount, pageFlags);

        LOG_INFO("MMAP syscall mapped %u64 bytes at virtual address %p (physical %p) with prot %u32 and flags %u32.", length, region.start, phys, prot, flags);
        return region.start;
    }
} // Syscalls