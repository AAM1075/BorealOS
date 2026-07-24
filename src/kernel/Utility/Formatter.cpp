#include "Formatter.h"

#include "StringView.h"

// This file defines format specifiers for the primitive types.
// And parsing for integers, and boolean values.

namespace Utility {
    const char digits[] = "0123456789ABCDEF";

    void WriteChar(Formatter::Writer out, char value) {
        if ((unsigned char)value - 32 <= 94 || value == '\n' || value == '\t' || value == '\r') {
            out(&value, 1);
        }
        else out("?", 1);
    }

    void WriteUnsigned(Formatter::Writer out, uint64_t value, uint64_t base) {
        if (value == 0) {
            WriteChar(out, '0');
            return;
        }
        char tmp[64]; // must fit 64 digits for base 2
        size_t n = 0;
        while (value > 0) {
            tmp[n++] = digits[value % base];
            value /= base;
        }
        while (n > 0)
            WriteChar(out, tmp[--n]);
    }

    void WriteSigned(Formatter::Writer out, int64_t value) {
        if (value < 0) {
            out("-", 1);
            // negate via unsigned to avoid UB on INT64_MIN
            WriteUnsigned(out, (uint64_t)(-(uint64_t)value), 10);
            return;
        }
        WriteUnsigned(out, (uint64_t)value, 10);
    }

    union DoubleBits {
        double d;
        uint64_t bits;
    };

    static const uint64_t doubleExpMask = 0x7FF0000000000000ULL;
    static const uint64_t doubleMantMask = 0x000FFFFFFFFFFFFFULL;
    static const uint64_t doubleSignMask = 0x8000000000000000ULL;

    bool IsNan(double value) {
        DoubleBits u{value};
        return (u.bits & doubleExpMask) == doubleExpMask && (u.bits & doubleMantMask) != 0;
    }
    bool IsInf(double value) {
        DoubleBits u{value};
        return (u.bits & doubleExpMask) == doubleExpMask && (u.bits & doubleMantMask) == 0;
    }
    bool SignBit(double value) {
        DoubleBits u{value};
        return (u.bits & doubleSignMask) != 0;
    }

    void WriteDouble(Formatter::Writer out, double value) {
        if (IsNan(value)) { out("nan", 3); return; }

        if (IsInf(value)) {
            if (SignBit(value)) out("-", 1);
            out("inf", 3);
            return;
        }

        if (SignBit(value)) {
            out("-", 1);
            value = -value;
        }

        auto intPart = static_cast<uint64_t>(value);
        WriteUnsigned(out, intPart, 10);
        out(".", 1);
        double frac = value - (double)intPart;
        for (int i = 0; i < 6; ++i) {
            frac *= 10.0;
            int digit = (int)frac;
            WriteChar(out, digits[digit]);
            frac -= digit;
        }
    }

    void WriteFloat(Formatter::Writer out, float value) {
        WriteDouble(out, (double)value);
    }

    // Unsigned numbers:

    template<>
    void Formatter::WriteValue(Writer out, uint8_t value) {
        WriteUnsigned(out, value, 10);
    }

    template<>
    void Formatter::WriteValue(Writer out, uint16_t value) {
        WriteUnsigned(out, value, 10);
    }

    template<>
    void Formatter::WriteValue(Writer out, uint32_t value) {
        WriteUnsigned(out, value, 10);
    }

    template<>
    void Formatter::WriteValue(Writer out, uint64_t value) {
        WriteUnsigned(out, value, 10);
    }

    template<>
    void Formatter::WriteValue(Writer out, void* value) {
        out("0x", 2);
        WriteUnsigned(out, (uintptr_t)value, 16);
    }

    // Signed numbers:

    template<>
    void Formatter::WriteValue(Writer out, int8_t value) {
        WriteSigned(out, value);
    }

    template<>
    void Formatter::WriteValue(Writer out, int16_t value) {
        WriteSigned(out, value);
    }

    template<>
    void Formatter::WriteValue(Writer out, int32_t value) {
        WriteSigned(out, value);
    }

    template<>
    void Formatter::WriteValue(Writer out, int64_t value) {
        WriteSigned(out, value);
    }

    // Floating point numbers:

    template<>
    void Formatter::WriteValue(Writer out, float value) {
        WriteFloat(out, value);
    }

    template<>
    void Formatter::WriteValue(Writer out, double value) {
        WriteDouble(out, value);
    }

    // Bool and char:

    template<>
    void Formatter::WriteValue(Writer out, bool value) {
        if (value) out("true", 4);
        else out("false", 5);
    }

    template<>
    void Formatter::WriteValue(Writer out, char value) {
        WriteChar(out, value);
    }

    // String:

    template<>
    void Formatter::WriteValue(Writer out, const char* value) {
        out(value, strlen(value));
    }

    template<>
    void Formatter::WriteValue(Writer out, StringView value) {
        out(value.Data(), value.Size());
    }

    // Parsing:

    int8_t DigitValue(char c) {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
        if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
        return -1;
    }

    template<>
    bool Formatter::Parse(StringView src, bool& value) {
        if (src.Size() == 0)
            return false;

        if (src == "1") {
            value = true;
            return true;
        }

        if (src == "0") {
            value = false;
            return true;
        }

        if (src == "true") {
            value = true;
            return true;
        }

        if (src == "false") {
            value = false;
            return true;
        }

        // No match
        return false;
    }

    bool ParseInteger(StringView src, uint64_t& value, uint64_t base) {
        if (src.Size() == 0)
            return false;

        uint64_t result = 0;
        for (size_t i = 0; i < src.Size(); i++) {
            int8_t d = DigitValue(src.Data()[i]);
            if (d < 0 || (uint64_t)d >= base) {
                return false;
            }

            if (result > (UINT64_MAX - (uint64_t)d) / base) {
                return false;
            }

            result = result * base + (uint64_t)d;
        }

        value = result;
        return true;
    }

    template<>
    bool Formatter::Parse(StringView src, uint64_t& value) {
        if (src.Size() == 0)
            return false;

        size_t start = 0;
        uint64_t base = 10;

        if (src.Size() >= 2) {
            if (src.Substring(0, 2) == "0x" || src.Substring(0,2) == "0X") {
                start = 2;
                base = 16;
            }
        }

        if (start >= src.Size())
            return false;

        return ParseInteger(src.Substring(start), value, base);
    }

    template<>
    bool Formatter::Parse(StringView src, int64_t& value) {
        if (src.Size() == 0)
            return false;

        bool negative = false;
        size_t start = 0;

        if (src.Substring(0,1) == "-" || src.Substring(0,1) == "+") {
            negative = src.Substring(0,1) == "-";
            start = 1;
        }

        if (start >= src.Size())
            return false;


        uint64_t mag;
        if (!ParseInteger(src.Substring(start), mag, 10))
            return false;

        uint64_t limit = negative ? (uint64_t)INT64_MAX + 1 : (uint64_t)INT64_MAX;
        if (mag > limit)
            return false;

        value = negative ? (int64_t)(0 - mag) : (int64_t)mag;
        return true;
    }
}