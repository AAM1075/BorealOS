#ifndef BOREALOS_CSTRING_H
#define BOREALOS_CSTRING_H

#include <Definitions.h>

extern "C" {
    int strlen(const char* str);
    int strcmp(const char *s1, const char *s2);
    int strncmp(const char *s1, const char *s2, size_t n);
    char* strchr(const char *s, int c);
    char* strstr(const char *haystack, const char *needle);
    void strncpy(char *dest, const char *src, size_t n);
}

#endif //BOREALOS_CSTRING_H
