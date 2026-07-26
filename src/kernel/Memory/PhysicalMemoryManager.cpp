#include "PhysicalMemoryManager.h"
#include <Logging.h>

#include "Boot/LimineDefinitions.h"
#include "Utility/Math.h"

namespace Memory {
    void PhysicalMemoryManager::Initialize() {
        _lock = Threading::Spinlock();

        auto memmap = Boot::Limine::MemmapRequest.response;
        auto hhdm = Boot::Limine::HhdmRequest.response;

        if (!memmap)
            PANIC("Memory map not available");

        if (!hhdm)
            PANIC("HHDM not available");

        struct {
            uint64_t addr;
            uint64_t length;
        } validRegions[memmap->entry_count];

        uint64_t validRegionCount = 0;
        uint64_t endAddr = 0;

        for (uint64_t i = 0; i < memmap->entry_count; i++) {
            auto entry = memmap->entries[i];

            if (entry->type != LIMINE_MEMMAP_USABLE) continue;
            if (entry->base + entry->length <= 1 * Constants::MiB) continue;

            validRegions[validRegionCount].addr = entry->base;
            validRegions[validRegionCount].length = entry->length;

            if (endAddr < entry->base + entry->length)
                endAddr = entry->base + entry->length;

            validRegionCount++;
        }

        _usableFrames = endAddr / Constants::PageSize;
        _bitmapSize = (_usableFrames + 7) / 8;
        _bitmapSize = Utility::Math::AlignUp(_bitmapSize, Constants::PageSize);

        LOG_DEBUG("Usable frames: {}, Bitmap size: {} bytes", _usableFrames, _bitmapSize);

        uint64_t requiredMapStorageSize = _bitmapSize * 2; // one reserved, one allocatable bitmap.
        void* bitmapMemory = nullptr;

        for (uint64_t regionIndex = 0; regionIndex < validRegionCount; regionIndex++) {
            auto start = Utility::Math::AlignUp(validRegions[regionIndex].addr, Constants::PageSize);
            auto end = Utility::Math::AlignDown(validRegions[regionIndex].addr + validRegions[regionIndex].length, Constants::PageSize);
            auto size = (end > start) ? (end - start) : 0;

            if (size >= requiredMapStorageSize) {
                bitmapMemory = reinterpret_cast<void*>(start);
                validRegions[regionIndex].addr += requiredMapStorageSize;
                validRegions[regionIndex].length -= requiredMapStorageSize;
                break;
            }
        }

        if (!bitmapMemory)
            PANIC("No valid memory region found for the bitmaps");

        _allocatableBitmap = (uint8_t*)bitmapMemory;
        _reservedBitmap = (uint8_t*)bitmapMemory + _bitmapSize;

        // Mark everything as reserved in both arrays, since we'd rather have less memory but no accidental overwrites
        for (uint64_t byteIndex = 0; byteIndex < _bitmapSize; byteIndex++) {
            (_allocatableBitmap + hhdm->offset)[byteIndex] = 0xFF;
            (_reservedBitmap + hhdm->offset)[byteIndex] = 0xFF;
        }

        for (uint64_t regionIndex = 0; regionIndex < validRegionCount; regionIndex++) {
            auto start = Utility::Math::AlignUp(validRegions[regionIndex].addr, Constants::PageSize);
            auto end = Utility::Math::AlignDown(validRegions[regionIndex].addr + validRegions[regionIndex].length, Constants::PageSize);

            for (uint64_t addr = start; addr < end; addr += Constants::PageSize) {
                uint64_t frameIndex = addr / Constants::PageSize;
                uint64_t byteIndex = frameIndex / 8;
                uint8_t bitIndex = frameIndex % 8;

                (_reservedBitmap + hhdm->offset)[byteIndex] &= ~(1 << bitIndex);
            }
        }

        // Mark the first 1MB as reserved
        for (uint64_t page = 0; page < (1 * Constants::MiB) / Constants::PageSize; page++) {
            uint64_t byteIndex = page / 8;
            uint8_t bitIndex = page % 8;

            (_reservedBitmap + hhdm->offset)[byteIndex] |= (1 << bitIndex);
        }

        // Reserve the 2 bitmaps
        auto bitmapStart = (uint64_t)((uintptr_t)bitmapMemory / Constants::PageSize);
        auto bitmapEnd = Utility::Math::AlignUp(bitmapStart + requiredMapStorageSize, Constants::PageSize) / Constants::PageSize;

        for (uint64_t page = bitmapStart; page < bitmapEnd; page++) {
            uint64_t byteIndex = page / 8;
            uint8_t bitIndex = page % 8;

            (_reservedBitmap + hhdm->offset)[byteIndex] |= (1 << bitIndex);
        }

        // Clear the entire allocatable bitmap, since we always check the reserved one BEFORE the allocatable one.
        for (uint64_t i = 0; i < _bitmapSize; i++) {
            (_allocatableBitmap + hhdm->offset)[i] = 0x00;
        }

        _frameCount = _bitmapSize * 8;
        _usableFrames = 0;

        for (uint64_t i = 0; i < _frameCount; i++) {
            uint64_t byteIndex = i / 8;
            uint8_t bitIndex = i % 8;

            if (!((_reservedBitmap + hhdm->offset)[byteIndex] & (1 << bitIndex))) {
                _usableFrames++;
            }
        }

        if (!TestAllocationAndFreeing()) {
            PANIC("Failed test case");
        }

        LOG_INFO("Device has {} MiB, with {} MiB available", (_frameCount * Constants::PageSize) / (Constants::MiB), (_usableFrames * Constants::PageSize) / (Constants::MiB));
    }

    uintptr_t PhysicalMemoryManager::AllocatePages(size_t pageCount) {
        Threading::ScopedLock lock(_lock, true);
        if (pageCount == 0) return 0;

        size_t totalPages = _frameCount;
        size_t runStart = 0;
        size_t runLength = 0;

        for (size_t page = 0; page < totalPages; page++) {
            if (!IsPageReserved(page) && !IsPageAllocated(page)) {
                if (runLength == 0)
                    runStart = page;

                runLength++;
                if (runLength == pageCount) {
                    break;
                }
            } else {
                runLength = 0;
            }
        }

        if (runLength < pageCount) {
            LOG_ERROR("Failed to allocate {} pages, not enough contiguous free pages available", pageCount);
            return 0;
        }

        for (size_t page = runStart; page < runStart + pageCount; page++) {
            size_t byteIndex = page / 8;
            uint8_t bitIndex = page % 8;

            (_allocatableBitmap + Boot::Limine::HhdmRequest.response->offset)[byteIndex] |= (1 << bitIndex);
        }

        _usableFrames -= pageCount;

        return (runStart * Constants::PageSize);
    }

    void PhysicalMemoryManager::FreePages(uintptr_t start, size_t pageCount) {
        Threading::ScopedLock lock(_lock, true);

        if (start % Constants::PageSize != 0) {
            PANIC("Invalid page start address");
            return;
        }

        size_t startPage = start / Constants::PageSize;
        size_t endPage = startPage + pageCount;

        for (size_t page = startPage; page < endPage; page++) {
            size_t byteIndex = page / 8;
            uint8_t bitIndex = page % 8;

            if (byteIndex >= _bitmapSize) {
                PANIC("Page index out of bounds");
            }

            if (IsPageReserved(page)) {
                PANIC("Attempted to free a reserved page");
            }

            if (!IsPageAllocated(page)) {
                PANIC("Attempted to free a page that is not allocated");
            }

            (_allocatableBitmap + Boot::Limine::HhdmRequest.response->offset)[byteIndex] &= ~(1 << bitIndex);
        }

        _usableFrames += pageCount;
    }

    bool PhysicalMemoryManager::TestAllocationAndFreeing() {
        uintptr_t onePage = AllocatePages(1);
        uintptr_t twoPages = AllocatePages(2);

        size_t amount = 10;
        uintptr_t megabytes = AllocatePages(amount * Constants::MiB / Constants::PageSize);

        if (!onePage) {
            LOG_ERROR("Failed to allocate 1 page");
        }

        if (!twoPages) {
            LOG_ERROR("Failed to allocate 2 pages");
        }

        if (!megabytes) {
            LOG_ERROR("Failed to allocate {} MB", amount);
        }

        LOG_DEBUG("Allocated 1 page at {}, 2 at {}, and {} megabytes at {}", (void*)onePage, (void*)twoPages, amount, (void*)megabytes);

        FreePages(onePage, 1);
        FreePages(twoPages, 2);
        FreePages(megabytes, amount * Constants::MiB / Constants::PageSize);

        // Reallocate each page, it should produce the exact same result as before.
        uintptr_t oldOnePage = onePage, oldTwoPages = twoPages, oldFiveMegabytes = megabytes;
        onePage = 0; twoPages = 0; megabytes = 0;

        onePage = AllocatePages(1);
        twoPages = AllocatePages(2);
        megabytes = AllocatePages(amount * Constants::MiB / Constants::PageSize);

        if (onePage != oldOnePage) {
            LOG_ERROR("Reallocated 1 page at {}, expected {}", (void*)onePage, (void*)oldOnePage);
        }
        if (twoPages != oldTwoPages) {
            LOG_ERROR("Reallocated 2 pages at {}, expected {}", (void*)twoPages, (void*)oldTwoPages);
        }
        if (megabytes != oldFiveMegabytes) {
            LOG_ERROR("Reallocated {} MB at {}, expected {}", amount, (void*)megabytes, (void*)oldFiveMegabytes);
        }

        FreePages(onePage, 1);
        FreePages(twoPages, 2);
        FreePages(megabytes, amount * Constants::MiB / Constants::PageSize);

        LOG_DEBUG("Successful test!");

        return true;
    }

    constexpr bool PhysicalMemoryManager::IsPageReserved(size_t page) const {
        size_t byteIndex = page / 8;
        uint8_t bitIndex = page % 8;

        if (byteIndex >= _bitmapSize) {
            PANIC("Page index out of bounds");
        }

        if ((_reservedBitmap + Boot::Limine::HhdmRequest.response->offset)[byteIndex] & (1 << bitIndex)) {
            return true;
        }

        return false;
    }

    constexpr bool PhysicalMemoryManager::IsPageAllocated(size_t page) const {
        size_t byteIndex = page / 8;
        uint8_t bitIndex = page % 8;

        if (byteIndex >= _bitmapSize) {
            PANIC("Page index out of bounds");
        }

        if ((_allocatableBitmap + Boot::Limine::HhdmRequest.response->offset)[byteIndex] & (1 << bitIndex)) {
            return true;
        }

        return false;
    }
} // Memory