#include "StringUtils.h"

bool Utility::StringUtils::IsCharWhitespace(const char c) {
    return (
        c == ' '  ||
        c == '\t' ||
        c == '\n' ||
        c == '\v' ||
        c == '\f' ||
        c == '\r'
    );
}
