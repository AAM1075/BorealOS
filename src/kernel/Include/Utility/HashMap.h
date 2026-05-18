#ifndef BOREALOS_HASHMAP_H
#define BOREALOS_HASHMAP_H

#include <Definitions.h>

#include "HashProvider.h"
#include "List.h"
#include "Optional.h"
#include "Move.h"

namespace Utility {

    template<typename K, typename V>
    class HashMap {
    public:
        explicit HashMap(size_t capacity = 16) : _entries(capacity), _capacity(capacity) {_entries.Reserve(_capacity);}
        ~HashMap() = default;

        void Insert(K&& key, V&& value) {
            if (_size >= _capacity / 2) { // Load factor of 0.5
                Resize();
            }

            size_t hash = Utility::HashProvider<K>()(key);
            size_t index = hash % _capacity;

            while (_entries[index].occupied && !_entries[index].deleted) {
                if (_entries[index].key == key) {
                    _entries[index].value = value; // Update existing key
                    return;
                }
                index = (index + 1) % _capacity;
            }

            _entries[index] = {key, value, true, false};
            _size++;
        }

        void Insert(const K& key, const V& value) {
            Insert(K(key), V(value));
        }

        Optional<V> Get(const K& key) const {
            size_t hash = Utility::HashProvider<K>()(key);
            size_t index = hash % _capacity;

            while (_entries[index].occupied) {
                if (!_entries[index].deleted && _entries[index].key == key) {
                    return Optional<V>(_entries[index].value);
                }
                index = (index + 1) % _capacity;
            }

            return Optional<V>(); // Not found
        }

        Optional<V> Get(const size_t hash) const {
            size_t index = hash % _capacity;

            while (_entries[index].occupied) {
                if (!_entries[index].deleted && Utility::HashProvider<K>()(_entries[index].key) == hash) {
                    return Optional<V>(_entries[index].value);
                }
                index = (index + 1) % _capacity;
            }

            return Optional<V>(); // Not found
        }

        void Remove(const K& key) {
            size_t hash = Utility::HashProvider<K>()(key);
            size_t index = hash % _capacity;

            while (_entries[index].occupied) {
                if (!_entries[index].deleted && _entries[index].key == key) {
                    _entries[index].deleted = true;

                    // Run the destructors
                    _entries[index].key.~K();
                    _entries[index].value.~V();

                    _size--;
                    return;
                }
                index = (index + 1) % _capacity;
            }
        }

        [[nodiscard]] size_t Size() const {
            return _size;
        }

        [[nodiscard]] size_t Capacity() const {
            return _capacity;
        }

        Optional<V> operator[](const K& key) const {
            return Get(key);
        }

    private:
        struct Entry {
            K key;
            V value;
            bool occupied = false;
            bool deleted = false;
        };

        Utility::List<Entry> _entries;
        size_t _size = 0;
        size_t _capacity = 16;

        void Resize() {
            _capacity *= 2;
            Utility::List<Entry> newEntries(_capacity);

            for (auto& entry : _entries) {
                if (entry.occupied && !entry.deleted) {
                    // Rehash the entry into the new list
                    size_t hash = Utility::HashProvider<K>()(entry.key);
                    size_t index = hash % _capacity;

                    while (newEntries[index].occupied) {
                        index = (index + 1) % _capacity; // Linear probing
                    }

                    newEntries[index] = {Utility::Move(entry.key), Utility::Move(entry.value), true, false};
                }
            }

            _entries = Utility::Move(newEntries);
        }
    };
}

#endif //BOREALOS_HASHMAP_H
