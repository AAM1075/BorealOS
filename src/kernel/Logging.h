#ifndef BOREALOS_LOGGING_H
#define BOREALOS_LOGGING_H

#include <Utility/Formatter.h>
#include <Utility/Traits.h>
#include <Utility/ANSI.h>

enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARNING = 2,
    ERROR = 3,
    FATAL = 4 // or panic.
};

namespace Logging {
    bool LogMessage(LogLevel);

    template<typename... Args>
    void LogFmt(LogLevel level, Utility::StringView file, Utility::Formatter::Writer out, Utility::Formatter::FormatString<Utility::Traits::TypeIdentityT<Args>...> message, Args&&... args) {
        if (!LogMessage(level))
            return;

        Utility::StringView color = Utility::ANSI::Colors::Foreground::White;
        Utility::StringView defaultColor = Utility::ANSI::Colors::Foreground::White;

        Utility::StringView levelStr = "UNKNOWN";

        if (level == LogLevel::DEBUG) {
            color = Utility::ANSI::Colors::Foreground::Cyan;
            levelStr = "DEBUG";
        } else if (level == LogLevel::INFO) {
            color = Utility::ANSI::Colors::Foreground::Green;
            levelStr = "INFO";
        } else if (level == LogLevel::WARNING) {
            color = Utility::ANSI::Colors::Foreground::Yellow;
            levelStr = "WARNING";
        } else if (level == LogLevel::ERROR) {
            color = Utility::ANSI::Colors::Foreground::Red;
            levelStr = "ERROR";
        } else if (level == LogLevel::FATAL) {
            color = Utility::ANSI::Colors::Foreground::Red;
            levelStr = "FATAL";
        }

        // [level] [file:line] - message

        out("[", 1);
        out(color.Data(), color.Size());
        out(levelStr.Data(), levelStr.Size());
        out(defaultColor.Data(), defaultColor.Size());
        out("] ", 2);

        out(file.Data(), file.Size());

        out(" - ", 3);

        Utility::Formatter::Format(out, message, static_cast<Args&&>(args)...);
    }
}

#define STRINGIFY_HELPER(x) #x
#define STRINGIFY(x) STRINGIFY_HELPER(x)

#define LOG_MESSAGE(level, file, msg, ...) do { \
    if (Logging::LogMessage(level)) { \
        Logging::LogFmt(level, file, [](const char* str, size_t c) { Core::Write(str, c); }, msg __VA_OPT__(,) __VA_ARGS__); \
        Core::Write("\n\r", 2); \
    } \
} while(0)

#define LOG_DEBUG(msg, ...) LOG_MESSAGE(LogLevel::DEBUG, "[" __FILE_NAME__ ":" STRINGIFY(__LINE__)"]", msg, __VA_ARGS__)
#define LOG_INFO(msg, ...) LOG_MESSAGE(LogLevel::INFO, "[" __FILE_NAME__ ":" STRINGIFY(__LINE__)"]", msg, __VA_ARGS__)
#define LOG_WARNING(msg, ...) LOG_MESSAGE(LogLevel::WARNING, "[" __FILE_NAME__ ":" STRINGIFY(__LINE__)"]", msg, __VA_ARGS__)
#define LOG_ERROR(msg, ...) LOG_MESSAGE(LogLevel::ERROR, "[" __FILE_NAME__ ":" STRINGIFY(__LINE__)"]", msg, __VA_ARGS__)
#define LOG_FATAL(msg, ...) LOG_MESSAGE(LogLevel::FATAL, "[" __FILE_NAME__ ":" STRINGIFY(__LINE__)"]", msg, __VA_ARGS__)

#endif //BOREALOS_LOGGING_H
