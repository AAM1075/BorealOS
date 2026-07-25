#ifndef BOREALOS_PHYSICALMEMORYMANAGER_H
#define BOREALOS_PHYSICALMEMORYMANAGER_H

#include <Definitions.h>

#include "Threading/Spinlock.h"

namespace Memory {
    class PhysicalMemoryManager {
    public:
        void Initialize();
        uintptr_t AllocatePages(size_t pageCount);
        void FreePages(uintptr_t start, size_t pageCount);

    private:
        Threading::Spinlock _lock;

        size_t _frameCount{};
        size_t _usableFrames{};
        size_t _bitmapSize{};

        uint8_t* _allocatableBitmap{};
        uint8_t* _reservedBitmap{};

        bool TestAllocationAndFreeing();
        [[nodiscard]] constexpr bool IsPageReserved(size_t page) const;
        [[nodiscard]] constexpr bool IsPageAllocated(size_t page) const;
    };
} // Memory

#endif //BOREALOS_PHYSICALMEMORYMANAGER_H
