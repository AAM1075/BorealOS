#include "Kernel.h"
#include "IO/Serial.h"

void Kernel::Initialize() {
    IO::Serial::Initialize(IO::Serial::COM1);
    Data.framebufferConsole.Initialize();
    PRINT("Initialized RS232 port COM1.");
}

void Kernel::Start() {

}

void Kernel::Log(const char *message) {
    IO::Serial::WriteString(IO::Serial::COM1, message);
    Data.framebufferConsole.Write(message);
}

void Kernel::Panic(const char *message) {
    asm("cli");

    IO::Serial::WriteString(IO::Serial::COM1, message);
    Data.framebufferConsole.Write("Kernel panic: ");
    Data.framebufferConsole.Write(message);

    for (;;)
        asm("hlt");
}

Kernel & Kernel::GetInstance() {
    static Kernel instance;
    return instance;
}

void Core::Print(const char *message) {
    Kernel::GetInstance().Log(message);
}

[[noreturn]] void Core::Panic(const char *message) {
    Kernel::GetInstance().Panic(message);
}
