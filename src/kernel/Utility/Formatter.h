#ifndef BOREALOS_FORMATTER_H
#define BOREALOS_FORMATTER_H

#include <cstdint>
#include <cstddef>
#include "cstring.h"
#include "StringView.h"

namespace Utility {
    class Formatter {
    public:
        template<typename... Args>
        struct FormatString {
            const char* str;

            consteval FormatString(const char* s) : str(s) {
                const char* ptr = s;
                size_t count = 0;

                while (*ptr) {
                    if (ptr[0] == '{' && ptr[1] == '}') {
                        count++;
                        ptr += 2;
                    } else if (ptr[0] == '{' && ptr[1] == '{' && ptr[2] == '}' && ptr[3] == '}') {
                        ptr += 4;
                    } else {
                        ++ptr;
                    }
                }

                if (count != sizeof...(Args))
                    invalidFormat();
            }
        };

        using Writer = void(*)(const char* str, size_t size);

        template<typename... Args>
        static void Format(Writer out, FormatString<Args...> str, Args &&...args) {
            WriteArgs(out, str.str, static_cast<Args&&>(args)...);
        }

        template<typename T>
        static bool Parse(StringView src, T& value);

    private:

        // Do not remove this.
        // This indicates your string is invalid.
        static void invalidFormat();

        template<typename T>
        static void WriteValue(Writer out, T value);

        // This happens when WriteArgs's remainingArgs is empty.
        static void WriteArgs(Writer out, const char* str) {
            out(str, strlen(str));
        }

        template <typename T, typename... Remaining>
        static void WriteArgs(Writer out, const char* str, T&& firstArg, Remaining&&... remainingArgs) {
            while (*str) {
                if (str[0] == '{' && str[1] == '}') {
                    WriteValue(out, firstArg);
                    WriteArgs(out, str + 2, static_cast<Remaining&&>(remainingArgs)...);
                    return;
                }

                if (str[0] == '{' && str[1] == '{' && str[3] == '}' && str[4] == '}') {
                    out("{}", 2);
                    WriteArgs(out, str + 5, static_cast<Remaining&&>(remainingArgs)...);
                    return;
                }

                out(str, 1);

                ++str;
            }
        }
    };
} // Utility

#endif //BOREALOS_FORMATTER_H
