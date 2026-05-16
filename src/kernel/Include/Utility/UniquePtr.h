#ifndef BOREALOS_UNIQUEPTR_H
#define BOREALOS_UNIQUEPTR_H

#include <Definitions.h>

#include "Move.h"

// std::unique_ptr like smart pointer

namespace Utility {
    template<typename T>
    class UniquePtr {
    public:
        explicit UniquePtr(T* ptr = nullptr) : _ptr(ptr) {}
        ~UniquePtr() {
            delete _ptr;
        }

        UniquePtr(const UniquePtr&) = delete;
        UniquePtr& operator=(const UniquePtr&) = delete;

        UniquePtr(UniquePtr&& other) noexcept : _ptr(other._ptr) {
            other._ptr = nullptr;
        }

        UniquePtr& operator=(UniquePtr&& other) noexcept {
            if (this == &other) {
                return *this; // Self-assignment check
            }

            delete _ptr; // Free the existing resource

            _ptr = other._ptr; // Transfer ownership
            other._ptr = nullptr; // Prevent double deletion

            return *this;
        }

        [[nodiscard]] T* Get() const {
            return _ptr;
        }

        T* operator->() const {
            return _ptr;
        }

        T& operator*() const {
            return *_ptr;
        }

    private:
        T* _ptr;
    };

    template<typename T, typename... Args>
    UniquePtr<T> MakeUnique(Args&&... args) {
        return UniquePtr<T>(Utility::MoveConstruct<T>(Utility::Move(args)...));
    }
}

#endif //BOREALOS_UNIQUEPTR_H
