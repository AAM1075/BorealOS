#ifndef BOREALOS_STRINGVIEW_H
#define BOREALOS_STRINGVIEW_H

#include <Definitions.h>
#include "String.h"

namespace Utility {
    // A non-owning view into a string, basically just a pointer and a length. This is used for functions that return substrings or modified versions of strings without actually allocating new strings.
    class StringView {
    public:
        StringView() : _data(""), _size(0) {}
        StringView(const char* data, size_t size) : _data(data), _size(size) {}
        StringView(const String& str) : _data(str.CStr()), _size(str.Size()) {}
        StringView(const char* data) : _data(data), _size(StringFormatter::strlen(data)) {}

        [[nodiscard]] const char* Data() const {
            return _data;
        }

        [[nodiscard]] size_t Size() const {
            return _size;
        }

        // Copies the contents of this StringView into a new String and returns it.
        [[nodiscard]] String ToString() const {
            return {*this};
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

    private:
        const char* _data;
        size_t _size;
    };

    template<>
    struct HashProvider<StringView> {
        uint32_t operator()(const StringView& str) const {
            uint32_t hash = 2166136261u;
            for (size_t i = 0; i < str.Size(); i++) {
                hash ^= static_cast<uint32_t>(str.Data()[i]);
                hash *= 16777619u;
            }
            return hash;
        }
    };
}

#endif //BOREALOS_STRINGVIEW_H
