#ifndef BOREALOS_KERNELDATA_H
#define BOREALOS_KERNELDATA_H

#include <Logging.h>

#include "Interrupts/GDT.h"
#include "Interrupts/IDT.h"
#include "IO/FramebufferConsole.h"
#include "Memory/Paging.h"
#include "Memory/PhysicalMemoryManager.h"
#include "Utility/CommandLineExtractor.h"
#include "IO/Serial.h"
#include "CPU/CPU.h"

struct KernelData {

    IO::Serial debugPort{IO::Serial::COM1};
    IO::FramebufferConsole framebufferConsole;
    Utility::CommandLineExtractor commandLineExtractor;
    LogLevel minimumLogLevel;
    Memory::PhysicalMemoryManager physicalMemoryManager;
    Memory::Paging paging{physicalMemoryManager};
    Interrupts::IDT idt{paging};
};

struct CpuData {
    uint32_t cpuId;
    Memory::Paging::State *pageState;
};

#endif //BOREALOS_KERNELDATA_H
