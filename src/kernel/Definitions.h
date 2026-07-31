#ifndef BOREALOS_DEFINITIONS_H
#define BOREALOS_DEFINITIONS_H

#include <cstdint>
#include <cstddef>

#define PACKED __attribute__((packed))
#define ALIGNED(x) __attribute__((aligned(x)))
#define UNUSED __attribute__((unused))
#define SET_BIT(x, n) ((x) |= (1U << (n)))
#define CLEAR_BIT(p, n) ((p) &= ~(1U << (n)))

namespace Core {
    void Write(const char* message, size_t c);
    [[noreturn]] void Panic(const char* message);
}

namespace Constants {
    constexpr uint64_t KiB = 1024;
    constexpr uint64_t MiB = KiB * 1024;
    constexpr uint64_t GiB = MiB * 1024;
    constexpr uint64_t TiB = GiB * 1024;
    constexpr uint64_t PiB = TiB * 1024;
}

namespace Architecture {
    constexpr uint64_t MaxCPUs = 1024; // TODO: if somehow this is too little add more
    constexpr uint64_t MaxIOAPICs = 16;
    constexpr uint64_t PageSize = 4 * Constants::KiB;
    constexpr uint64_t KernelOffset = 0xFFFFFFFF80000000;
    extern volatile uintptr_t *StackBottom;
    extern volatile uintptr_t *StackTop;
    extern volatile uintptr_t *KernelBase;
    extern volatile uintptr_t *KernelEnd;
    extern volatile uintptr_t *DefaultFaultHandlerTop;
    extern volatile uintptr_t *DefaultFaultHandlerBottom;
    extern volatile size_t StackSize;
    extern volatile size_t KernelSize;
}

#define PANIC(message) Core::Panic(message)

#endif //BOREALOS_DEFINITIONS_H
