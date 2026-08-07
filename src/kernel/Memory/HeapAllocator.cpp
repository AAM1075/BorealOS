#include "HeapAllocator.h"
#include "Logging.h"
#include "Utility/Math.h"

namespace Memory {
    HeapAllocator::HeapAllocator(PhysicalMemoryManager &pmm, Paging &paging) : _pmm(pmm), _paging(paging) {
    }

    static HeapAllocator* globalHeapAllocator = nullptr;

    HeapAllocator * HeapAllocator::GetInstance() {
        return globalHeapAllocator;
    }

    int HeapAllocator::GetSizeClassIndex(size_t objectSize) const {
        for (uint8_t sizeIndex = 0; sizeIndex < 8; sizeIndex++) {
            if (objectSizes[sizeIndex] >= objectSize) return sizeIndex;
        }

        // The size is not within our defined min and max
        return -1;
    }

    size_t HeapAllocator::GetClosestSizeMatch(size_t objectSize, size_t objectAlignment) {
        // Align the object first
        objectSize = Utility::Math::Max(objectSize, objectAlignment);

        // Round up to the nearest power of 2
        objectSize--;
        objectSize |= objectSize >> 1;
        objectSize |= objectSize >> 2;
        objectSize |= objectSize >> 4;
        objectSize |= objectSize >> 8;
        objectSize |= objectSize >> 16;
        objectSize++;

        return objectSize;
    }

    uintptr_t HeapAllocator::Allocate(size_t objectSize, size_t objectAlignment) {
        if (objectSize == 0) return 0;
        Threading::ScopedLock lock(_heapLock, true);

        // Enforce a minimum size of 16 and get the closest size class that can hold this object
        objectSize = Utility::Math::Max(objectSize, static_cast<size_t>(objectSizes[0]));
        objectSize = GetClosestSizeMatch(objectSize, objectAlignment);
        int sizeIndex = GetSizeClassIndex(objectSize);

        // Traverse the header chain, starting at the root header
        SmallAllocSlabHeader* currentHeader = initialHeaderPages[sizeIndex];
        SmallAllocSlabHeader* tailHeader = nullptr;

        while (currentHeader != nullptr) {
            tailHeader = currentHeader;

            for (uint16_t objectIndex = 0; objectIndex < currentHeader->objectCount; objectIndex++) {
                uint8_t qwordIndex = objectIndex / 64;
                uint8_t bitIndex = objectIndex % 64;

                // Check if this object is available for allocation
                if ((currentHeader->allocationBitmap[qwordIndex] & (1ULL << bitIndex)) == 0) {
                    // Mark this object as allocated and return it's virtual address
                    currentHeader->allocationBitmap[qwordIndex] |= (1ULL << bitIndex);
                    return currentHeader->slabAddress + (objectIndex * currentHeader->objectSize);
                }
            }

            currentHeader = reinterpret_cast<SmallAllocSlabHeader*>(currentHeader->nextHeaderPageAddress);
        }

        // All slabs are currently full for this object size, so it's time to allocate a new header
        // Compare the start of the page and the current tail header's address to see if any more headers can fit in this page
        uintptr_t pageStart = reinterpret_cast<uintptr_t>(tailHeader) & ~(Architecture::PageSize - 1);
        bool pageHasRoom = (reinterpret_cast<uintptr_t>(tailHeader) + sizeof(SmallAllocSlabHeader)) < (pageStart + Architecture::PageSize);
        SmallAllocSlabHeader* newHeader = nullptr;

        // If this page has room, we can just create a new header and data page and then retry the allocation
        if (pageHasRoom) {
            // Create the new header and initialize it's properties
            newHeader = tailHeader + 1;
            newHeader->objectSize = objectSize;
            newHeader->objectCount = tailHeader->objectCount;
            newHeader->nextHeaderPageAddress = 0;
        }
        else {
            // Otherwise, we'll have to allocate a new page
            // Allocate a new physical page for this size class's headers
            uintptr_t headerPhysicalAddress = _pmm.AllocatePages(1);
            Paging::AvailableVirtualAddressRange headerVAddrRange = _paging.FindAvailableVirtualAddressRangeKernel(1);
            _paging.MapPage(_paging.GetKernelState(), headerVAddrRange.start, headerPhysicalAddress, Paging::Flags::ReadWrite);

            // Zero out the entire header page to get rid of any garbage memory
            __builtin_memset(reinterpret_cast<void*>(headerVAddrRange.start), 0, Architecture::PageSize);

            // Configure the new header
            newHeader = reinterpret_cast<SmallAllocSlabHeader*>(headerVAddrRange.start);
            newHeader->objectSize = objectSize;
            newHeader->objectCount = tailHeader->objectCount;
            newHeader->nextHeaderPageAddress = 0;
        }

        // Zero the allocation bitmap since RAM can contain garbage data on power-on
        for (auto& word : newHeader->allocationBitmap) {
            word = 0;
        }

        // Allocate a new data page for this header
        uintptr_t newDataPagePhysicalAddress = _pmm.AllocatePages(1);
        Paging::AvailableVirtualAddressRange newDataPageVAddrRange = _paging.FindAvailableVirtualAddressRangeKernel(1);
        _paging.MapPage(_paging.GetKernelState(), newDataPageVAddrRange.start, newDataPagePhysicalAddress, Paging::Flags::ReadWrite);
        newHeader->slabAddress = newDataPageVAddrRange.start;

        // Assign the new header's address to the tail header so it knows that this is the next header in the chain
        tailHeader->nextHeaderPageAddress = reinterpret_cast<uintptr_t>(newHeader);

        // Retry the allocation
        newHeader->allocationBitmap[0] |= (1ULL << 0);
        return newHeader->slabAddress;
    }

    void HeapAllocator::Free(uintptr_t objectAddress) {
        if (objectAddress == 0) return;
        Threading::ScopedLock lock(_heapLock, true);

        // Iterate through all the size classes to find which pool owns this address
        for (auto currentHeader : initialHeaderPages) {
            while (currentHeader != nullptr) {
                if (objectAddress >= currentHeader->slabAddress && objectAddress < currentHeader->slabAddress + Architecture::PageSize) {
                    // We found the correct pool, now we need to calculate the object index based on its offset in the slab
                    uintptr_t objectOffset = objectAddress - currentHeader->slabAddress;
                    uint16_t objectIndex = objectOffset / currentHeader->objectSize;

                    // If the index is within bounds, clear the bit to mark it as free
                    if (objectIndex < currentHeader->objectCount) {
                        uint8_t qwordIndex = objectIndex / 64;
                        uint8_t bitIndex = objectIndex % 64;
                        currentHeader->allocationBitmap[qwordIndex] &= ~(1ULL << bitIndex);
                        return;
                    }
                }

                currentHeader = reinterpret_cast<SmallAllocSlabHeader*>(currentHeader->nextHeaderPageAddress);
            }
        }
    }

    void HeapAllocator::Initialize() {
        // Allocate pages to store headers for each size class
        for (uint8_t sizeIndex = 0; sizeIndex < 8; sizeIndex++) {
            const uint16_t objectSize = objectSizes[sizeIndex];

            // Allocate a physical page for this size class's headers
            uintptr_t headerPhysicalAddress = _pmm.AllocatePages(1);
            Paging::AvailableVirtualAddressRange headerVAddrRange = _paging.FindAvailableVirtualAddressRangeKernel(1);
            _paging.MapPage(_paging.GetKernelState(), headerVAddrRange.start, headerPhysicalAddress, Paging::Flags::ReadWrite);

            // Zero out the entire header page to get rid of any garbage memory
            __builtin_memset(reinterpret_cast<void*>(headerVAddrRange.start), 0, Architecture::PageSize);

            // Treat the new page as an array of SmallAllocSlabHeader
            auto* headerArray = reinterpret_cast<SmallAllocSlabHeader*>(headerVAddrRange.start);
            initialHeaderPages[sizeIndex] = &headerArray[0];

            // Initialize the root header's parameters
            initialHeaderPages[sizeIndex]->objectSize = objectSize;
            initialHeaderPages[sizeIndex]->objectCount = Architecture::PageSize / objectSize;
            initialHeaderPages[sizeIndex]->nextHeaderPageAddress = 0;

            // Allocate the initial data slab for this size class
            uintptr_t slabPhysicalAddress = _pmm.AllocatePages(1);
            Paging::AvailableVirtualAddressRange slabVAddrRange = _paging.FindAvailableVirtualAddressRangeKernel(1);
            _paging.MapPage(_paging.GetKernelState(), slabVAddrRange.start, slabPhysicalAddress, Paging::Flags::ReadWrite);

            initialHeaderPages[sizeIndex]->slabAddress = slabVAddrRange.start;
        }

        globalHeapAllocator = this;
        LOG_INFO("Heap allocator initialized");
    }
} // Memory

void* operator new(size_t size) { return reinterpret_cast<void*>(Memory::HeapAllocator::GetInstance()->Allocate(size, 8)); }
void* operator new[](size_t size) { return reinterpret_cast<void*>(Memory::HeapAllocator::GetInstance()->Allocate(size, 8)); }

void operator delete(void* ptr, size_t size) noexcept { operator delete(ptr); }
void operator delete[](void* ptr, size_t size) noexcept { operator delete[](ptr); }

void operator delete(void* ptr) noexcept {
    if (!ptr) return;
    Memory::HeapAllocator::GetInstance()->Free(reinterpret_cast<uintptr_t>(ptr));
}

void operator delete[](void* ptr) noexcept {
    if (!ptr) return;
    Memory::HeapAllocator::GetInstance()->Free(reinterpret_cast<uintptr_t>(ptr));
}