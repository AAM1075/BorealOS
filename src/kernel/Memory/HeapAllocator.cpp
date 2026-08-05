#include "HeapAllocator.h"
#include "Logging.h"

namespace Memory {
    HeapAllocator* HeapAllocator::activeHeapAllocator = nullptr;

    HeapAllocator::HeapAllocator(PhysicalMemoryManager& pmm, Paging& paging)
        : _pmm(pmm), _paging(paging), _heapStart(nullptr), _totalHeapBytes(0), _allocatedBytes(0) {
        activeHeapAllocator = this;
    }

    void HeapAllocator::Initialize() {
        // Get virtual addresses for the heap
        size_t initialPages = DEFAULT_HEAP_SIZE / Architecture::PageSize;
        Paging::AvailableVirtualAddressRange addrRange = _paging.FindAvailableVirtualAddressRangeKernel(initialPages);
        _heapStart = reinterpret_cast<BlockHeader*>(addrRange.start);
        uintptr_t currentVirtAddr = addrRange.start;

        // Allocate the virtual addresses via the PMM
        for (size_t pageIndex = 0; pageIndex < initialPages; pageIndex++) {
            uintptr_t physFrame = _pmm.AllocatePages(1);
            if (!physFrame) {
                PANIC("Failed to allocate physical pages for heap init!");
            }

            _paging.MapPage(_paging.GetKernelState(), currentVirtAddr, physFrame, Paging::Flags::ReadWrite);
            currentVirtAddr += Architecture::PageSize;
        }

        // Set up the root block header
        size_t totalSize = initialPages * Architecture::PageSize;
        _heapStart->size = totalSize - sizeof(BlockHeader);
        _heapStart->isFree = true;
        _heapStart->nextBlock = nullptr;
        _totalHeapBytes = totalSize;
        _allocatedBytes = 0;

        LOG_DEBUG("Allocated {} page(s) ({} KiB) for the heap", initialPages, (initialPages * Architecture::PageSize) / Constants::KiB);
        LOG_INFO("Heap initialized at {}", (void*)_heapStart);
    }

    void* HeapAllocator::Allocate(size_t size, Paging::Flags flags) {
        if (size == 0) return nullptr;

        // Force an 8-byte alignment for safety
        size = (size + 7) & ~7;

        BlockHeader* currentBlock = _heapStart;
        BlockHeader* lastBlock = nullptr;
        while (currentBlock != nullptr) {
            lastBlock = currentBlock;
            if (currentBlock->isFree && currentBlock->size >= size) {
                // Should we split this block?
                if (currentBlock->size > size + sizeof(BlockHeader) + 8) {
                    BlockHeader* nextBlock = reinterpret_cast<BlockHeader*>(
                        reinterpret_cast<uintptr_t>(currentBlock + 1) + size
                    );

                    nextBlock->size = currentBlock->size - size - sizeof(BlockHeader);
                    nextBlock->isFree = true;
                    nextBlock->nextBlock = currentBlock->nextBlock;

                    currentBlock->size = size;
                    currentBlock->nextBlock = nextBlock;
                }

                currentBlock->isFree = false;
                _allocatedBytes += currentBlock->size;
                return currentBlock + 1;
            }

            currentBlock = currentBlock->nextBlock;
        }

        // No existing free block fit the request, we need to allocate a new page
        size_t pagesNeeded = (size + sizeof(BlockHeader) + Architecture::PageSize - 1) / Architecture::PageSize;
        if (pagesNeeded < 1) pagesNeeded = 1;

        // Find virtual space and map new physical pages
        Paging::AvailableVirtualAddressRange addrRange = _paging.FindAvailableVirtualAddressRangeKernel(pagesNeeded);
        uintptr_t currentVirt = addrRange.start;

        for (size_t i = 0; i < pagesNeeded; ++i) {
            uintptr_t physFrame = _pmm.AllocatePages(1);
            if (!physFrame) {
                LOG_ERROR("Failed to allocate physical page for heap expansion!");
                return nullptr;
            }

            _paging.MapPage(_paging.GetKernelState(), currentVirt, physFrame, flags);
            currentVirt += Architecture::PageSize;
        }

        size_t newChunkSize = pagesNeeded * Architecture::PageSize;
        _totalHeapBytes += newChunkSize;

        // Initialize the new block header and attach it to the end of the linked list
        BlockHeader* newBlock = reinterpret_cast<BlockHeader*>(addrRange.start);
        newBlock->size = newChunkSize - sizeof(BlockHeader);
        newBlock->isFree = true;
        newBlock->nextBlock = nullptr;
        if (lastBlock != nullptr) lastBlock->nextBlock = newBlock;

        // Retry the allocation recursively using the newly added space
        return Allocate(size, flags);
    }

    void HeapAllocator::Free(uintptr_t objectAddress) {
        if (objectAddress == 0) return;

        // Step backward from the user pointer to find the header, then mark it as free
        BlockHeader* header = reinterpret_cast<BlockHeader*>(objectAddress) - 1;
        header->isFree = true;
        _allocatedBytes -= header->size;

        // If the next block is also free, glue them together to prevent memory fragmentation
        if (header->nextBlock != nullptr && header->nextBlock->isFree) {
            header->size += sizeof(BlockHeader) + header->nextBlock->size;
            header->nextBlock = header->nextBlock->nextBlock;
        }

        // Shrink the heap if we have unused pages
        Shrink();
    }

    void HeapAllocator::Shrink() {
        if (_heapStart == nullptr) return;

        BlockHeader* currentBlock = _heapStart;
        BlockHeader* previousBlock = nullptr;

        // Find the last block in the list
        while (currentBlock->nextBlock != nullptr) {
            previousBlock = currentBlock;
            currentBlock = currentBlock->nextBlock;
        }

        // Check if that final block is free
        if (currentBlock->isFree) {
            size_t blockSizeWithHeader = currentBlock->size + sizeof(BlockHeader);

            // We can only release memory if the free block is large enough to fully cover one or more RAM pages
            // NOTE: The block size must also be an exact multiple of the page size, this ensures that we respect page boundaries
            if (blockSizeWithHeader % Architecture::PageSize != 0) return;
            size_t pagesToFree = blockSizeWithHeader / Architecture::PageSize;

            if (pagesToFree > 0) {
                uintptr_t blockVirtAddr = reinterpret_cast<uintptr_t>(currentBlock);
                size_t bytesToRelease = pagesToFree * Architecture::PageSize;

                // Unmap the pages and return them to the PMM
                for (size_t i = 0; i < pagesToFree; ++i) {
                    uintptr_t targetVirt = blockVirtAddr + (i * Architecture::PageSize);
                    uintptr_t physAddr = _paging.GetPhysicalAddress(_paging.GetKernelState(), targetVirt);
                    if (physAddr != 0) _pmm.FreePages(physAddr, 1);

                    _paging.UnmapPage(_paging.GetKernelState(), targetVirt);
                }

                // Cut the tail off of the list
                if (previousBlock != nullptr) previousBlock->nextBlock = nullptr;
                else return;

                _totalHeapBytes -= bytesToRelease;
            }
        }
    }

    size_t HeapAllocator::GetAllocatedBytes() { return _allocatedBytes; }
} // Memory

// Overrides for the 'new' & 'delete' operators
void* operator new(size_t size) { return Memory::HeapAllocator::activeHeapAllocator->Allocate(size, Memory::Paging::Flags::ReadWrite); }
void* operator new[](size_t size) { return Memory::HeapAllocator::activeHeapAllocator->Allocate(size, Memory::Paging::Flags::ReadWrite); }
void operator delete(void* ptr) noexcept {
    if (ptr) {
        Memory::HeapAllocator::activeHeapAllocator->Free(reinterpret_cast<uintptr_t>(ptr));
    }
}

void operator delete[](void* ptr) noexcept {
    if (ptr) {
        Memory::HeapAllocator::activeHeapAllocator->Free(reinterpret_cast<uintptr_t>(ptr));
    }
}

void operator delete(void* ptr, size_t) noexcept {
    if (ptr) {
        Memory::HeapAllocator::activeHeapAllocator->Free(reinterpret_cast<uintptr_t>(ptr));
    }
}

void operator delete[](void* ptr, size_t) noexcept {
    if (ptr) {
        Memory::HeapAllocator::activeHeapAllocator->Free(reinterpret_cast<uintptr_t>(ptr));
    }
}