#include "Kernel.h"
#include <Logging.h>

extern "C" [[noreturn]] void kmain() {
    auto& kernel = Kernel::GetInstance();

    kernel.Initialize();
    LOG_INFO("Kernel initialized successfully, starting main loop...");

    kernel.Start();

    PANIC("Kernel reached end of .Start()");
}
