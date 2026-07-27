#ifndef BOREALOS_PAGING_H
#define BOREALOS_PAGING_H

#include <Definitions.h>

#include "PhysicalMemoryManager.h"

namespace Memory {
    class Paging {
    private:
        struct PT { uint64_t entries[512]; } ALIGNED(Architecture::PageSize);
        struct PD { uint64_t entries[512]; } ALIGNED(Architecture::PageSize);
        struct PDP { uint64_t entries[512]; } ALIGNED(Architecture::PageSize);
        struct PML4 { uint64_t entries[512]; } ALIGNED(Architecture::PageSize);

    public:
        enum class Flags : uint64_t {
            Present = 1 << 0,
            ReadWrite = 1 << 1,
            User = 1 << 2,
            WriteThrough = 1 << 3,
            CacheDisable = 1 << 4,
            Accessed = 1 << 5,
            Dirty = 1 << 6,
            HugePage = 1 << 7,
            Global = 1 << 8,
            NoExecute = 1ULL << 63,
        };

        struct State {
            PML4 *pml4{};
            Threading::Spinlock lock{};
        };

        struct AvailableVirtualAddressRange {
            uint64_t start;
            uint64_t end;
        };

        explicit Paging(PhysicalMemoryManager& pmm);

        void Initialize();

        void MapPage(State *state, uint64_t virtualAddress, uint64_t physicalAddress, Flags flags);
        void UnmapPage(State *state, uint64_t virtualAddress);

        uint64_t GetPhysicalAddress(State *state, uint64_t virtualAddress);
        [[nodiscard]] bool IsMapped(State *state, uint64_t virtualAddress);

        void SwitchToKernelPageTable();
        static void SwitchToPageTable(State *state);

    private:
        Threading::Spinlock _lock;
        PhysicalMemoryManager& _pmm;
        State _kernelState;

        static constexpr uint64_t PointerMask = 0x000FFFFFFFFFF000; // Mask to get the address portion of a page table entry
        static constexpr uint64_t FlagsMask = 0xFFF0000000000FFF; // Mask to get the flags portion of a page table entry

        void ReserveHigherHalfPML4Slots(State *state, uint64_t higherHalf) const;
        void CopyExistingPageTableToNew(State *state, uint64_t higherHalf);
        void DeepCopyPageTables(uint32_t level, uintptr_t srcPhys, uintptr_t dstPhys, uint64_t higherHalf);

        static bool IsTableEmpty(const uint64_t *table);
        static inline void ExtractPageTableIndices(uint64_t vaddr, uint32_t& pml4Index, uint32_t& pdpIndex, uint32_t& pdIndex, uint32_t& ptIndex, uint32_t& pageIndex);
    };

    constexpr Paging::Flags operator|(Paging::Flags a, Paging::Flags b) {
        return static_cast<Paging::Flags>(static_cast<uint64_t>(a) | static_cast<uint64_t>(b));
    }

    constexpr Paging::Flags operator|=(Paging::Flags& a, Paging::Flags b) {
        a = a | b;
        return a;
    }
} // Memory

#endif //BOREALOS_PAGING_H
