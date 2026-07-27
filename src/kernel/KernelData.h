#ifndef BOREALOS_KERNELDATA_H
#define BOREALOS_KERNELDATA_H

#include <Logging.h>

#include "IO/FramebufferConsole.h"
#include "Memory/PhysicalMemoryManager.h"
#include "Utility/CommandLineExtractor.h"
#include "IO/Serial.h"

struct KernelData {
    IO::Serial debugPort{IO::Serial::COM1};
    IO::FramebufferConsole framebufferConsole;
    Utility::CommandLineExtractor commandLineExtractor;
    LogLevel minimumLogLevel;
    Memory::PhysicalMemoryManager physicalMemoryManager;
};

#endif //BOREALOS_KERNELDATA_H
