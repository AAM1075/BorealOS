#ifndef BOREALOS_MATH_H
#define BOREALOS_MATH_H

namespace Utility::Math {
    template<typename T>
    [[nodiscard]] constexpr T Min(T a, T b) {
        return a < b ? a : b;
    }

    template<typename T>
    [[nodiscard]] constexpr T Max(T a, T b) {
        return a > b ? a : b;
    }

    template<typename T>
    [[nodiscard]] constexpr T Clamp(T value, T min, T max) {
        return Max(min, Min(max, value));
    }
}

#endif //BOREALOS_MATH_H
