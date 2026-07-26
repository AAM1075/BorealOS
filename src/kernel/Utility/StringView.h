#ifndef BOREALOS_STRINGVIEW_H
#define BOREALOS_STRINGVIEW_H

#include "cstring.h"

namespace Utility {
    // A non-owning view into a string, basically just a pointer and a length. This is used for functions that return substrings or modified versions of strings without actually allocating new strings.
    class StringView {
    public:
        static constexpr size_t npos = static_cast<size_t>(-1);

        constexpr StringView() : _data(""), _size(0) {}
        constexpr StringView(const char* data, size_t size) : _data(data), _size(size) {}
        constexpr StringView(const char* data) : _data(data), _size(Utility::Strings::StringLength(data)) {}

        [[nodiscard]] const char* Data() const {
            return _data;
        }

        [[nodiscard]] size_t Size() const {
            return _size;
        }

        // Equality operator
        bool operator==(const StringView& other) const {
            if (_size != other._size) {
                return false;
            }
            return strncmp(_data, other._data, _size) == 0;
        }

        // Inequality operator
        bool operator!=(const StringView& other) const {
            return !(*this == other);
        }

        // Compare against C string
        int Compare(const char * path) const {
            return strncmp(_data, path, _size);
        }

        [[nodiscard]] StringView Substring(size_t start, size_t length) const {
            if (start >= _size)
                return {_data + _size, 0};

            return {_data + start, length};
        }

        [[nodiscard]] StringView Substring(size_t start) const {
            if (start >= _size)
                return {_data + _size, 0};

            return {_data + start, _size - start};
        }

        [[nodiscard]] size_t CountOf(StringView needle) const {
            // Count how many times the needle is found in haystack (this)
            size_t count = 0;
            for (size_t i = 0; i < _size; i++) {
                if (strncmp(_data + i, needle._data, needle._size) == 0) {
                    count++;
                    i += needle._size - 1; // Move past the found needle
                }
            }
            return count;
        }

        [[nodiscard]] size_t IndexOf(StringView needle) const {
            for (size_t i = 0; i <= _size - needle._size; i++) {
                if (strncmp(_data + i, needle._data, needle._size) == 0) {
                    return i;
                }
            }

            return npos;
        }

    private:
        const char* _data;
        size_t _size;
    };
}

#endif //BOREALOS_STRINGVIEW_H
