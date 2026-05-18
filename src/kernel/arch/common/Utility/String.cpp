#include <Utility/String.h>
#include <Utility/StringView.h>

#include "Utility/Math.h"

namespace Utility {
    String::String(const StringView &view) {
        _size = view.Size();
        if (_size <= SSO_CAPACITY) {
            memcpy(_data.ssoData, view.Data(), _size);
            _data.ssoData[_size] = '\0';
        } else {
            _data.heapData = new uint8_t[_size + 1];
            memcpy(_data.heapData, view.Data(), _size);
            _data.heapData[_size] = '\0';
        }
    }

    StringView String::SubStringView(size_t start, size_t length) const {
        if (start >= _size) {
            return {"", 0}; // Out of bounds, return an empty string view
        }

        size_t maxLength = _size - start;
        if (length > maxLength) {
            length = maxLength; // Adjust length to fit within the string
        }

        return {CStr() + start, length};
    }

    StringView String::SubStringView(size_t start) const {
        return SubStringView(start, _size - start);
    }

    StringView String::AsView() const {
        return {CStr(), _size};
    }

    String String::ToLower() const {
        String result(*this);
        char* data = const_cast<char*>(result.CStr());
        for (size_t i = 0; i < _size; i++) {
            if (data[i] >= 'A' && data[i] <= 'Z') {
                data[i] += ('a' - 'A'); // Convert to lowercase
            }
        }
        return result;
    }

    String String::ToUpper() const {
        String result(*this);
        char* data = const_cast<char*>(result.CStr());
        for (size_t i = 0; i < _size; i++) {
            if (data[i] >= 'a' && data[i] <= 'z') {
                data[i] -= ('a' - 'A'); // Convert to uppercase
            }
        }
        return result;
    }

    int String::Compare(const String &other) const {
        return strcmp(CStr(), other.CStr());
    }

    int String::CompareIgnoreCase(const String &other) const {
        const char* a = CStr();
        const char* b = other.CStr();

        for (size_t i = 0; i < _size && i < other._size; i++) {
            char charA = a[i];
            char charB = b[i];

            // Convert to lowercase for comparison
            if (charA >= 'A' && charA <= 'Z') {
                charA += ('a' - 'A');
            }
            if (charB >= 'A' && charB <= 'Z') {
                charB += ('a' - 'A');
            }

            if (charA != charB) {
                return (charA < charB) ? -1 : 1;
            }
        }

        // If we reached the end of one of the strings, the shorter string is considered smaller
        if (_size == other._size) {
            return 0;
        }
        return (_size < other._size) ? -1 : 1;
    }

    bool String::StartsWith(const String &prefix) const {
        if (prefix._size > _size) {
            return false; // Prefix is longer than the string, can't start with it
        }

        return strncmp(CStr(), prefix.CStr(), prefix._size) == 0;
    }

    bool String::EndsWith(const String &suffix) const {
        if (suffix._size > _size) {
            return false; // Suffix is longer than the string, can't end with it
        }

        return strncmp(CStr() + (_size - suffix._size), suffix.CStr(), suffix._size) == 0;
    }

    bool String::Contains(const String &substring) const {
        if (substring._size == 0) {
            return true; // An empty substring is considered to be contained in any string
        }

        if (substring._size > _size) {
            return false; // Substring is longer than the string, can't be contained in it
        }

        for (size_t i = 0; i <= _size - substring._size; i++) {
            if (strncmp(CStr() + i, substring.CStr(), substring._size) == 0) {
                return true;
            }
        }

        return false;
    }

    size_t String::Find(const String &substring) const {
        if (substring._size == 0) {
            return 0; // An empty substring is considered to be found at the beginning of the string
        }

        if (substring._size > _size) {
            return -1; // Substring is longer than the string, can't be found in it
        }

        for (size_t i = 0; i <= _size - substring._size; i++) {
            if (strncmp(CStr() + i, substring.CStr(), substring._size) == 0) {
                return i;
            }
        }

        return -1; // Not found
    }

    size_t String::RFind(const String &substring) const {
        if (substring._size == 0) {
            return _size; // An empty substring is considered to be found at the end of the string
        }

        if (substring._size > _size) {
            return -1; // Substring is longer than the string, can't be found in it
        }

        for (size_t i = _size - substring._size + 1; i-- > 0;) {
            if (strncmp(CStr() + i, substring.CStr(), substring._size) == 0) {
                return i;
            }
        }

        return -1; // Not found
    }

    uint32_t String::Hash() {
        if (_hashCode != 0) {
            return _hashCode; // Return cached hash code if available
        }

        uint32_t hash = 2166136261u;
        for (size_t i = 0; i < _size; i++) {
            hash ^= static_cast<uint8_t>(CStr()[i]);
            hash *= 16777619u;
        }
        _hashCode = hash;
        return hash;
    }

    bool String::IsEmpty() const {
        return _size == 0;
    }

    String String::Append(const String &other) const {
        String result;
        result._size = _size + other._size;
        if (result._size <= SSO_CAPACITY) {
            memcpy(result._data.ssoData, CStr(), _size);
            memcpy(result._data.ssoData + _size, other.CStr(), other._size);
            result._data.ssoData[result._size] = '\0';
        } else {
            result._data.heapData = new uint8_t[result._size + 1];
            memcpy(result._data.heapData, CStr(), _size);
            memcpy(result._data.heapData + _size, other.CStr(), other._size);
            result._data.heapData[result._size] = '\0';
        }
        return result;
    }

    String String::Prepend(const String &other) const {
        String result;
        result._size = _size + other._size;
        if (result._size <= SSO_CAPACITY) {
            memcpy(result._data.ssoData, other.CStr(), other._size);
            memcpy(result._data.ssoData + other._size, CStr(), _size);
            result._data.ssoData[result._size] = '\0';
        } else {
            result._data.heapData = new uint8_t[result._size + 1];
            memcpy(result._data.heapData, other.CStr(), other._size);
            memcpy(result._data.heapData + other._size, CStr(), _size);
            result._data.heapData[result._size] = '\0';
        }
        return result;
    }
}
