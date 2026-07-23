#include "Kernel.h"

extern "C" [[noreturn]] void kmain() {
    auto& kernel = Kernel::GetInstance();

    kernel.Initialize();
    PRINT("Kernel initialized successfully");

    kernel.Start();

    PANIC("Kernel reached end of .Start()");
}
