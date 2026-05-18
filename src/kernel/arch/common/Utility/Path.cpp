#include <Utility/Path.h>

namespace Utility {
    size_t Path::GetComponentCount(const char *path, char delimiter) {
        size_t count = 0;
        const char* p = path;

        while (*p) {
            if (p == path && *p == delimiter) {
                p++; // Skip leading delimiter
                continue;
            }

            while (*p == delimiter) p++; // Skip consecutive delimiters
            if (*p) {
                count++;
                while (*p && *p != delimiter) p++; // Move to the next delimiter
            }
        }

        return count;
    }

    size_t Path::GetMaxComponentLength(const char *path, char delimiter) {
        size_t maxLength = 0;
        size_t currentLength = 0;
        const char* p = path;

        while (*p) {
            if (*p == delimiter) {
                if (currentLength > maxLength) {
                    maxLength = currentLength;
                }
                currentLength = 0; // Reset for the next component
            } else {
                currentLength++;
            }
            p++;
        }

        // Check the last component
        if (currentLength > maxLength) {
            maxLength = currentLength;
        }

        return maxLength;
    }

    size_t Path::SplitPath(const char *path, char delimiter, const char **components) {
        size_t count = 0;
        const char* p = path;

        while (*p) {
            while (*p == delimiter) p++; // Skip consecutive delimiters
            if (*p) {
                components[count++] = p; // Store the start of the component
                while (*p && *p != delimiter) p++; // Move to the next delimiter
                if (*p) {
                    *const_cast<char*>(p) = '\0'; // Null-terminate the component
                    p++; // Move past the delimiter for the next iteration
                }
            }
        }

        return count;
    }

    void Path::SplitPath(const char *string, char c, StringView *array) {
        size_t index = 0;
        const char* p = string;

        // StringView is a non-owning view, which does not need a null terminator, so we can just set the data pointer and length
        while (*p) {
            while (*p == c) p++; // Skip consecutive delimiters
            if (*p) {
                const char* start = p; // Start of the component
                while (*p && *p != c) p++; // Move to the next delimiter
                array[index++] = StringView(start, p - start); // Create a StringView for the component
            }
        }
    }
}
