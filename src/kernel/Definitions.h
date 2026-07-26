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
    constexpr uint64_t PageSize = 4 * KiB;
}

#define PANIC(message) Core::Panic(message)

#endif //BOREALOS_DEFINITIONS_H
