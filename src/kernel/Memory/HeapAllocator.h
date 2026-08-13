#ifndef BOREALOS_HEAPALLOCATOR_H
#define BOREALOS_HEAPALLOCATOR_H

#include <Definitions.h>
#include "Paging.h"
#include "Threading/Spinlock.h"

namespace Memory {
    class HeapAllocator {
    public:
        uint16_t objectSizes[8] = { 16, 32, 64, 128, 256, 512, 1024, 2048 };

        explicit HeapAllocator(PhysicalMemoryManager& pmm, Paging& paging);
        static HeapAllocator* GetInstance();

        uintptr_t Allocate(size_t objectSize, size_t objectAlignment);
        uintptr_t Reallocate(size_t newObjectSize, size_t newObjectAlignment, uintptr_t oldObjectAddress, size_t oldObjectSize);
        void Free(uintptr_t objectAddress);
        void Initialize();

    private:
        struct alignas(64) SmallAllocSlabHeader {
            uintptr_t nextHeaderPageAddress{};
            uintptr_t slabAddress{};
            uint64_t allocationBitmap[4] = {}; // Tracks 256 objects, the maximum we can track per physical page with a minimum object size of 16 bytes
            uint16_t objectCount = 0;
            uint16_t objectSize = 0;
        };

        struct alignas(32) LargeAllocSlabHeader {
            uintptr_t nextHeaderPageAddress{};
            uintptr_t objectAddress{};
            uint16_t objectSize = 0;
        };

        struct HeapStats {
            uint64_t allocatedBytes = 0; // Bytes taken up by allocated objects
            uint64_t allocatedPages = 0;
            uint64_t heapSize = 0; // Total heap size in bytes (page count * page size)
        };

        LargeAllocSlabHeader* _largeAllocationHeaders = nullptr;
        SmallAllocSlabHeader* _initialHeaderPages[8]{};
        PhysicalMemoryManager& _pmm;
        Paging& _paging;
        Threading::Spinlock _heapLock;

        static size_t GetClosestSizeMatch(size_t objectSize, size_t objectAlignment);
        [[nodiscard]] int GetSizeClassIndex(size_t objectSize) const;
        HeapStats GetHeapStats();
    };
} // Memory

#endif //BOREALOS_HEAPALLOCATOR_H
