#ifndef BOREALOS_COMMANDLINEEXTRACTOR_H
#define BOREALOS_COMMANDLINEEXTRACTOR_H

#include "StringView.h"
#include <Boot/limine.h>
#include <Boot/LimineDefinitions.h>

#include "Optional.h"

// Parses the kernel_cmdline from limine, and returns a value.

namespace Utility {
    class CommandLineExtractor {
    private:
        struct limine_executable_cmdline_response *_response{};
        bool _hasCommandLine = false;

    public:
        void Initialize();

        template<typename T>
        // Gets either a bool, or integer (0xABC or 100/-100), other types will be supported later.
        // If an identifier is missing, and T is bool, you get false, if it is present you get IDENTIFIER=value or, if no =value exists true.
        // If an identifier is missing, and T is anything else, you get default Optional, so no value.
        // Bool is the only exception to that rule.
        Optional<T> GetValue(StringView identifier);
    };
}

#endif //BOREALOS_COMMANDLINEEXTRACTOR_H
