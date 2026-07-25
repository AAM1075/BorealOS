#include "Kernel.h"
#include "Parameters.h"
#include <Logging.h>

void Kernel::Initialize() {
    Data.framebufferConsole.Initialize();
    Data.commandLineExtractor.Initialize();

    auto logLevel = Data.commandLineExtractor.GetValue<uint64_t>(Parameters::LOG_LEVEL);
    if (logLevel.HasValue())
        Data.minimumLogLevel = static_cast<LOG_LEVEL>(logLevel.Value());
}

void Kernel::Start() {

}

void Kernel::Log(const char *message, size_t c) {
    Data.framebufferConsole.Write(message, c);
}

void Kernel::Panic(const char *message) {
    asm("cli");

    Data.framebufferConsole.Write("Kernel panic: ");
    Data.framebufferConsole.Write(message);

    for (;;)
        asm("hlt");
}

Kernel & Kernel::GetInstance() {
    static Kernel instance;
    return instance;
}

bool Logging::LogMessage(LOG_LEVEL level) {
    if (level >= LOG_LEVEL::ERROR) {
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
