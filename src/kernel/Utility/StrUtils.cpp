#include "StrUtils.h"

bool Utility::StrUtils::IsCharWhitespace(const char c) {
    return (
        c == ' '  ||
        c == '\t' ||
        c == '\n' ||
        c == '\v' ||
        c == '\f' ||
        c == '\r'
    );
}
