#ifndef BOREALOS_HEAPALLOCATOR_H
#define BOREALOS_HEAPALLOCATOR_H

#include <Definitions.h>
#include "Paging.h"

namespace Memory {
    class HeapAllocator {
    public:
        static constexpr uint64_t DEFAULT_HEAP_SIZE = 2 * Constants::MiB;

        explicit HeapAllocator(PhysicalMemoryManager& pmm, Paging& paging);

        static HeapAllocator* activeHeapAllocator;

        size_t GetAllocatedBytes();
        void* Allocate(size_t size, Paging::Flags flags);
        void Free(uintptr_t objectAddress);
        void Shrink();
        void Initialize();

    private:
        struct BlockHeader {
            size_t size; // Excludes the size of this header
            bool isFree;
            BlockHeader* nextBlock;
        };

        PhysicalMemoryManager& _pmm;
        Paging& _paging;

        BlockHeader* _heapStart;
        size_t _totalHeapBytes;
        size_t _allocatedBytes;
    };
} // Memory

#endif //BOREALOS_HEAPALLOCATOR_H
