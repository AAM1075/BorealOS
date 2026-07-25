#ifndef BOREALOS_OPTIONAL_H
#define BOREALOS_OPTIONAL_H

#include <Definitions.h>
#include "Traits.h"

namespace Utility {
    template<typename T>
    class Optional {
    public:
        Optional() : _hasValue(false) {}
        Optional(const T& value) : _hasValue(true), _value(value) {}
        Optional(T&& value) : _hasValue(true), _value(Utility::Traits::Move(value)) {}

        [[nodiscard]] bool HasValue() const {
            return _hasValue;
        }

        [[nodiscard]] const T& Value() const {
            if (!_hasValue) {
                PANIC("Attempted to access value of an empty Optional!");
            }
            return _value;
        }

        [[nodiscard]] T& Value() {
            if (!_hasValue) {
                PANIC("Attempted to access value of an empty Optional!");
            }
            return _value;
        }

        [[nodiscard]] T& ValueOr(T initializer) {
            if (_hasValue) {
                return _value;
            }
            _value = initializer;
            _hasValue = true;
            return _value;
        }

        void Reset() {
            _hasValue = false;
            _value = T(); // Reset the value to its default state
        }

        // Forward the . and -> operators to the underlying value, for convenience.
        const T* operator->() const {
            if (!_hasValue) {
                PANIC("Attempted to access value of an empty Optional!");
            }
            return &_value;
        }

        T* operator->() {
            if (!_hasValue) {
                PANIC("Attempted to access value of an empty Optional!");
            }
            return &_value;
        }

    private:
        bool _hasValue;
        T _value;
    };
}

#endif //BOREALOS_OPTIONAL_H
