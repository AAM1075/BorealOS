#ifndef BOREALOS_STRING_H
#define BOREALOS_STRING_H

#include <Definitions.h>

#include "StringFormatter.h"
#include "HashProvider.h"

namespace Utility {

    class StringView;

    // std::string like string class, but with less features.
    // Strings are and should be immutable, all operations that modify a string should return a new string instead of modifying the existing one.
    class String {
    public:
        static constexpr size_t SSO_CAPACITY = 15; // 15 bytes + 1 for null terminator.

        String() : _size(0) {
            _data.ssoData[0] = '\0'; // Null terminator for empty string
        }

        String(const char* str) {
            _size = StringFormatter::strlen(str);
            if (_size <= SSO_CAPACITY) {
                memcpy(_data.ssoData, str, _size);
                memset(_data.ssoData + _size, '\0', SSO_CAPACITY - _size + 1);
            } else {
                _data.heapData = new uint8_t[_size + 1];
                memcpy(_data.heapData, str, _size);
                _data.heapData[_size] = '\0';
            }
        }

        String(const String& other) {
            _size = other._size;
            if (_size <= SSO_CAPACITY) {
                memcpy(_data.ssoData, other._data.ssoData, _size + 1);
                memset(_data.ssoData + _size, '\0', SSO_CAPACITY - _size + 1);
            } else {
                _data.heapData = new uint8_t[_size + 1];
                memcpy(_data.heapData, other._data.heapData, _size);
                _data.heapData[_size] = '\0';
            }
        }

        String(String&& other) noexcept {
            _size = other._size;
            if (_size <= SSO_CAPACITY) {
                memcpy(_data.ssoData, other._data.ssoData, _size);
                memset(_data.ssoData + _size, '\0', SSO_CAPACITY - _size + 1);
            } else {
                _data.heapData = other._data.heapData;
                other._data.heapData = nullptr;
            }
        }

        String(const StringView& view);

        ~String() {
            if (_size > SSO_CAPACITY) {
                delete[] _data.heapData;
            }
        }

        String& operator=(const String& other) {
            if (this == &other) return *this;
            if (this->_hashCode > 0 && other._hashCode > 0 && this->_hashCode == other._hashCode) {
                return *this; // If the hash codes are the same and valid, we can assume the strings are the same and skip copying.
            }

            uint8_t* newHeapData = nullptr;
            if (other._size > SSO_CAPACITY) {
                newHeapData = new uint8_t[other._size + 1];
                if (!newHeapData)
                    return *this; // If we fail to allocate, just keep the old string and return.

                memcpy(newHeapData, other._data.heapData, other._size + 1);
            }

            // Now it is safe to release old resources
            if (_size > SSO_CAPACITY) {
                delete[] _data.heapData;
            }

            _size = other._size;
            if (_size <= SSO_CAPACITY) {
                memcpy(_data.ssoData, other._data.ssoData, _size + 1);
            } else {
                _data.heapData = newHeapData;
            }

            return *this;
        }

        String& operator=(String&& other) noexcept {
            if (this == &other) {
                return *this; // Self-assignment check
            }

            if (_size > SSO_CAPACITY) {
                delete[] _data.heapData;
            }

            _size = other._size;
            if (_size <= SSO_CAPACITY) {
                memcpy(_data.ssoData, other._data.ssoData, _size + 1);
            } else {
                _data.heapData = other._data.heapData;
                other._data.heapData = nullptr;
            }

            return *this;
        }

        [[nodiscard]] const char* CStr() const {
            return _size <= SSO_CAPACITY ? reinterpret_cast<const char*>(_data.ssoData) : reinterpret_cast<const char*>(_data.heapData);
        }

        [[nodiscard]] size_t Size() const {
            return _size;
        }

        // Equality
        [[nodiscard]] bool operator==(const String& other) const {
            if (_size != other._size) {
                return false;
            }

            if (_hashCode > 0 && other._hashCode > 0 && _hashCode != other._hashCode) {
                return false; // If the hash codes are different and valid, we can assume the strings are different and skip comparing.
            }

            return strncmp(CStr(), other.CStr(), _size) == 0;
        }

        [[nodiscard]] bool operator!=(const String& other) const {
            return !(*this == other);
        }

        // Basic operations
        [[nodiscard]] StringView SubStringView(size_t start, size_t length) const;
        [[nodiscard]] StringView SubStringView(size_t start) const;
        [[nodiscard]] StringView AsView() const;
        [[nodiscard]] String ToLower() const;
        [[nodiscard]] String ToUpper() const;
        [[nodiscard]] int Compare(const String& other) const;
        [[nodiscard]] int CompareIgnoreCase(const String& other) const;
        [[nodiscard]] bool StartsWith(const String& prefix) const;
        [[nodiscard]] bool EndsWith(const String& suffix) const;
        [[nodiscard]] bool Contains(const String& substring) const;
        [[nodiscard]] size_t Find(const String& substring) const; // Returns the index of the first occurrence of substring, or -1 if not found.
        [[nodiscard]] size_t RFind(const String& substring) const; // Returns the index of the last occurrence of substring, or -1 if not found.
        [[nodiscard]] uint32_t Hash(); // Returns a hash of the string, useful for hash maps and such.
        [[nodiscard]] bool IsEmpty() const;

        // Concatenation
        [[nodiscard]] String Append(const String& other) const;
        [[nodiscard]] String Prepend(const String& other) const;
        [[nodiscard]] String operator+(const String& other) const {
            return Append(other);
        }

        friend String operator+(const char* lhs, const String& rhs) {
            return String(lhs).Append(rhs);
        }

        friend String operator+(const String& lhs, const char* rhs) {
            return lhs.Append(String(rhs));
        }

    private:
        union {
            uint8_t ssoData[SSO_CAPACITY + 1]; // +1 for null terminator
            uint8_t* heapData;
        } _data;
        size_t _size;
        uint32_t _hashCode = 0;
    };

    template<>
    struct HashProvider<String> {
        uint32_t operator()(const String& str) const {
            return const_cast<String&>(str).Hash();
        }
    };

}

#endif //BOREALOS_STRING_H
