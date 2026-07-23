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
    void Print(const char* message);
    [[noreturn]] void Panic(const char* message);
}

#define PRINT(msg) do { Core::Print(msg); Core::Print("\n\r"); } while(0)
#define PANIC(msg) do { Core::Panic(msg); } while(0)

#endif //BOREALOS_DEFINITIONS_H
