#include "Kernel.h"

void Kernel::Initialize() {
    Data.framebufferConsole.Initialize();
}

void Kernel::Start() {

}

void Kernel::Log(const char *message) {
    Data.framebufferConsole.Write(message);
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

void Core::Print(const char *message) {
    Kernel::GetInstance().Log(message);
}

[[noreturn]] void Core::Panic(const char *message) {
    Kernel::GetInstance().Panic(message);
}
