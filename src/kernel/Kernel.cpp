#include "Kernel.h"
#include "IO/Serial.h"
#include "Parameters.h"
#include <Logging.h>

void Kernel::Initialize() {
    Data.debugPort.Initialize();
    Data.framebufferConsole.Initialize();
    Data.commandLineExtractor.Initialize();

    auto logLevel = Data.commandLineExtractor.GetValue<uint64_t>(Parameters::LOG_LEVEL);
    if (logLevel.HasValue())
        Data.minimumLogLevel = static_cast<LogLevel>(logLevel.Value());

    Data.physicalMemoryManager.Initialize();
}

void Kernel::Start() {

}

void Kernel::Log(const char *message, size_t c) {
    Data.debugPort.Write(message, c);
    Data.framebufferConsole.Write(message, c);
}

void Kernel::Panic(const char *message) {
    asm volatile ("cli");

    Data.debugPort.Write(message, strlen(message));
    Data.framebufferConsole.Write("Kernel panic: ");
    Data.framebufferConsole.Write(message);

    for (;;)
        asm("hlt");
}

Kernel & Kernel::GetInstance() {
    static Kernel instance;
    return instance;
}

bool Logging::LogMessage(LogLevel level) {
    if (level >= LogLevel::ERROR) {
        return true;
    }

    if (level >= Kernel::GetInstance().Data.minimumLogLevel) {
        return true;
    }

    return false;
}

void Core::Write(const char *message, size_t c) {
    Kernel::GetInstance().Log(message, c);
}

[[noreturn]] void Core::Panic(const char *message) {
    Kernel::GetInstance().Panic(message);
}
