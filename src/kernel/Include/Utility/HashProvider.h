#ifndef BOREALOS_HASHPROVIDER_H
#define BOREALOS_HASHPROVIDER_H

#include <Definitions.h>
#include <concepts>

namespace Utility {
    template<typename T>
    struct HashProvider {
        size_t operator()(const T& value) = delete; // By default, hashing is not supported for any type. Specialize this template for types that you want to be able to hash, such as String.
    };

    // Use the IsPrimitive type trait to provide a default hash function
    template<typename T>
    requires std::integral<T>
    struct HashProvider<T> {
        size_t operator()(const T& value) const {
            // A simple hash function for integers, you can replace this with a better one if you want.
            return static_cast<size_t>(value);
        }
    };
}

#endif //BOREALOS_HASHPROVIDER_H
