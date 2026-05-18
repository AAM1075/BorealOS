#ifndef BOREALOS_LIST_H
#define BOREALOS_LIST_H

#include <Definitions.h>
#include "Predicate.h"

namespace Utility {
    /// Basic dynamic array implementation.
    template<typename T>
    class List {
    public:
        List(size_t capacity = 16) : _size(0), _capacity(capacity) {
            _data = new T[_capacity];
        }

        ~List() {
            delete[] _data;
        }

        List(const List&) = delete;
        List& operator=(const List&) = delete;

        List(List&& other) {
            _size = other._size;
            _capacity = other._capacity;
            _data = other._data;

            other._size = 0;
            other._capacity = 0;
            other._data = nullptr;
        }

        List& operator=(List&& other) {
            if (this == &other) {
                return *this; // Self-assignment check
            }

            delete[] _data;

            _size = other._size;
            _capacity = other._capacity;
            _data = other._data;

            other._size = 0;
            other._capacity = 0;
            other._data = nullptr;

            return *this;
        }

        List() = delete;

        void Add(const T& item) {
            if (_size >= _capacity) {
                Resize(_capacity * 2);
            }
            _data[_size++] = item;
        }

        void Remove(size_t index) {
            if (index >= _size) {
                return; // Out of bounds
            }
            for (size_t i = index; i < _size - 1; i++) {
                _data[i] = _data[i + 1];
            }
            _size--;
        }

        void Remove(const T& item) {
            for (size_t i = 0; i < _size; i++) {
                if (_data[i] == item) {
                    Remove(i);
                    return;
                }
            }
        }

        T& operator[](size_t index) {
            if (index >= _size) {
                // Out of bounds, since this is a kernel, we should panic since this might be a vulnerability if we just return a reference to some random memory.
                PANIC("List index out of bounds!");
            }

            return _data[index];
        }

        const T& operator[](size_t index) const {
            if (index >= _size) {
                // Out of bounds, since this is a kernel, we should panic since this might be a vulnerability if we just return a reference to some random memory.
                PANIC("List index out of bounds!");
            }

            return _data[index];
        }

        size_t IndexOf(T thread) const {
            for (size_t i = 0; i < _size; i++) {
                if (_data[i] == thread) {
                    return i;
                }
            }
            return -1; // Not found
        }

        void Clear() {
            _size = 0;
        }

        // Resize (if necessary) and act as if we added newCapacity items.
        // This calls the default constructor for any new items, so it is not just a simple resize, it is more of a "reserve and default construct" function.
        void Reserve(size_t newCapacity) {
            size_t oldSize = _size;
            if (newCapacity > _capacity) {
                Resize(newCapacity);
            }

            _size = newCapacity;
            for (size_t i = oldSize; i < newCapacity; i++) {
                _data[i] = T();
            }
        }

        [[nodiscard]] size_t Size() const { return _size; }
        [[nodiscard]] size_t Capacity() const { return _capacity; }
        [[nodiscard]] T* begin() { return _data; }
        [[nodiscard]] T* end() { return _data + _size; }

        bool Any(Predicate<T> predicate) {
            for (size_t i = 0; i < _size; i++) {
                if (predicate(_data[i])) {
                    return true;
                }
            }
            return false;
        }

        bool All(Predicate<T> predicate) {
            for (size_t i = 0; i < _size; i++) {
                if (!predicate(_data[i])) {
                    return false;
                }
            }
            return true;
        }

        Optional<T> Find(Predicate<T> predicate) {
            for (size_t i = 0; i < _size; i++) {
                if (predicate(_data[i])) {
                    return Optional<T>(_data[i]);
                }
            }
            return Optional<T>(); // Not found
        }

        Optional<T> FindLast(Predicate<T> predicate) {
            for (size_t i = _size; i-- > 0;) {
                if (predicate(_data[i])) {
                    return Optional<T>(_data[i]);
                }
            }
            return Optional<T>(); // Not found
        }


    private:
        void Resize(size_t newCapacity) {
            auto newData = new T[newCapacity];
            memcpy(newData, _data, sizeof(T) * _size);
            delete[] _data;
            _data = newData;
            _capacity = newCapacity;
        }

        T* _data;
        size_t _size;
        size_t _capacity;
    };
}

#endif //BOREALOS_LIST_H