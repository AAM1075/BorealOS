#include "cstring.h"

extern "C" {
    int strlen(const char* str) {
        int count = 0;
        while (*str++) {
            count++;
        }
        return count;
    }

    int strcmp(const char *s1, const char *s2) {
        while (*s1 && (*s1 == *s2)) {
            s1++;
            s2++;
        }
        return *reinterpret_cast<const unsigned char *>(s1) - *reinterpret_cast<const unsigned char *>(s2);
    }

    int strncmp(const char *s1, const char *s2, size_t n) {
        for (size_t i = 0; i < n; i++) {
            if (s1[i] != s2[i]) {
                return static_cast<unsigned char>(s1[i]) - static_cast<unsigned char>(s2[i]);
            }
            if (s1[i] == '\0') {
                return 0;
            }
        }
        return 0;
    }

    char* strchr(const char *s, int c) {
        while (*s) {
            if (*s == c) {
                return const_cast<char *>(s);
            }
            s++;
        }
        return nullptr;
    }

    char* strstr(const char *haystack, const char *needle) {
        if (!*needle) {
            return const_cast<char *>(haystack);
        }

        for (; *haystack; haystack++) {
            if (*haystack == *needle) {
                const char *h = haystack, *n = needle;
                while (*h && *n && *h == *n) {
                    h++;
                    n++;
                }
                if (!*n) {
                    return const_cast<char *>(haystack);
                }
            }
        }
        return nullptr;
    }

    void strncpy(char *dest, const char *src, size_t n) {
        size_t i;
        for (i = 0; i < n && src[i] != '\0'; i++) {
            dest[i] = src[i];
        }
        for (; i < n; i++) {
            dest[i] = '\0';
        }
    }
}