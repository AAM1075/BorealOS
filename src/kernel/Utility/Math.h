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

    template<typename T>
    [[nodiscard]] constexpr T Abs(T value) {
        return value < 0 ? -value : value;
    }

    // todo: use a more efficient algorithm for this
    template<typename T>
    [[nodiscard]] constexpr T Pow(T base, T exponent) {
        if (exponent == 0) return 1;
        if (exponent < 0) return 1 / Pow(base, -exponent);
        T result = 1;
        for (T i = 0; i < exponent; i++) {
            result *= base;
        }
        return result;
    }
}

#endif //BOREALOS_MATH_H
