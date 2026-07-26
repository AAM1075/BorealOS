#include "CommandLineExtractor.h"

#include <Logging.h>

namespace Utility {
    // Arguments can be like 'IDENTIFIER IDENTIFIER=123 IDENTIFIER="test test test"'
    size_t SplitArguments(StringView src, StringView* out) {
        size_t count = 0;
        StringView split(" ");
        StringView quote("\"");
        size_t pos = 0;
        size_t previouspos = 0;

        bool quotes = false;
        while (pos <= src.Size()) {
            if (pos == src.Size()) {
                if (out) {
                    out[count] = src.Substring(previouspos, pos - previouspos);
                }
                break;
            }

            if (src.Substring(pos, split.Size()) == split && !quotes) {
                if (out) {
                    out[count] = src.Substring(previouspos, pos - previouspos);
                }
                pos++;
                previouspos = pos;

                while (src.Substring(pos, split.Size()) == split) {
                    pos++;
                }

                count++;
            }

            if (src.Substring(pos, split.Size()) == quote) {
                quotes = !quotes;
            }

            // Nothing was found, skip
            pos++;
        }

        return count + 1;
    }

    void CommandLineExtractor::Initialize() {
        _response = Boot::Limine::CommandlineRequest.response;
        if (!_response || strlen(_response->cmdline) == 0) {
            _hasCommandLine = false;
        }

        size_t argCount = SplitArguments(_response->cmdline, nullptr);
        StringView args[argCount];
        StringView uniqueIdentifiers[argCount];
        size_t uniqueCount = 0;
        SplitArguments(_response->cmdline, args);

        auto hasIdentifier = [](StringView ident, StringView* array, size_t arrayLen) -> bool {
            for (size_t i = 0; i < arrayLen; i++) {
                if (array[i] == ident)
                    return true;
            }

            return false;
        };

        for (size_t i = 0; i < argCount; i++) {
            StringView ident = args[i];
            if (ident.CountOf("=") > 0)
                ident = ident.Substring(0, ident.IndexOf("="));

            if (hasIdentifier(ident, uniqueIdentifiers, uniqueCount)) {
                LOG_ERROR("Multiple identifiers for '{}' detected, this is undefined behaviour!", ident);
            } else {
                uniqueIdentifiers[uniqueCount++] = ident;
            }
        }

        _hasCommandLine = true;
    }

    // If the identifier is available, without the = it is true.
    // If the identifier is available, with = the value must be either 1 or 0. Or it is unknown (Optional())
    template<>
    Optional<bool> CommandLineExtractor::GetValue(StringView identifier) {
        if (!_hasCommandLine) {
            return {};
        }

        StringView cmdline(_response->cmdline);

        size_t argCount = SplitArguments(cmdline, nullptr);
        StringView args[argCount];
        SplitArguments(cmdline, args);

        for (size_t i = 0; i < argCount; i++) {
            auto arg = args[i];
            if (arg == identifier) {
                return {true};
            }

            auto ident = arg.Substring(0, arg.IndexOf("="));
            if (ident != identifier) {
                continue;
            }

            if (arg.CountOf("=") > 0) {
                bool value;
                if (Formatter::Parse(arg.Substring(arg.IndexOf("=") + 1), value)) {
                    return {value};
                }

                return {};
            }
        }

        return {false};
    }

    template<>
    Optional<uint64_t> CommandLineExtractor::GetValue(StringView identifier) {
        if (!_hasCommandLine) {
            return {};
        }

        StringView cmdline(_response->cmdline);

        size_t argCount = SplitArguments(cmdline, nullptr);
        StringView args[argCount];
        SplitArguments(cmdline, args);

        for (size_t i = 0; i < argCount; i++) {
            auto arg = args[i];

            if (arg.IndexOf("=") == StringView::npos)
                continue;

            auto ident = arg.Substring(0, arg.IndexOf("="));
            if (ident != identifier) {
                continue;
            }

            if (arg.CountOf("=") > 0) {
                uint64_t value;
                if (Formatter::Parse(arg.Substring(arg.IndexOf("=") + 1), value)) {
                    return {value};
                }

                break;
            }
        }

        return {};
    }

    template<>
    Optional<int64_t> CommandLineExtractor::GetValue(StringView identifier) {
        if (!_hasCommandLine) {
            return {};
        }

        StringView cmdline(_response->cmdline);

        size_t argCount = SplitArguments(cmdline, nullptr);
        StringView args[argCount];
        SplitArguments(cmdline, args);

        for (size_t i = 0; i < argCount; i++) {
            auto arg = args[i];

            if (arg.IndexOf("=") == StringView::npos)
                continue;

            auto ident = arg.Substring(0, arg.IndexOf("="));
            if (ident != identifier) {
                continue;
            }

            if (arg.CountOf("=") > 0) {
                int64_t value;
                if (Formatter::Parse(arg.Substring(arg.IndexOf("=") + 1), value)) {
                    return {value};
                }

                break;
            }
        }

        return {};
    }

    template<>
    Optional<StringView> CommandLineExtractor::GetValue(StringView identifier) {
        if (!_hasCommandLine) {
            return {};
        }

        StringView cmdline(_response->cmdline);

        size_t argCount = SplitArguments(cmdline, nullptr);
        StringView args[argCount];
        SplitArguments(cmdline, args);

        for (size_t i = 0; i < argCount; i++) {
            auto arg = args[i];

            if (arg.IndexOf("=") == StringView::npos)
                continue;

            auto ident = arg.Substring(0, arg.IndexOf("="));
            if (ident != identifier) {
                continue;
            }

            auto value = arg.Substring(arg.IndexOf("=") + 1);
            if (value.Substring(0,1) == "\"") {
                value = value.Substring(1, value.Size() - 2); // strip the quotes
            }

            if (value.Size() > 0)
                return {value};
            return {};
        }

        return {};
    }
}
