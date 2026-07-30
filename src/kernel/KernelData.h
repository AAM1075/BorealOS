#ifndef BOREALOS_KERNELDATA_H
#define BOREALOS_KERNELDATA_H

#include <Logging.h>

#include "Firmware/ACPI.h"
#include "Interrupts/GDT.h"
#include "Interrupts/IDT.h"
#include "Interrupts/IOAPIC.h"
#include "Interrupts/LAPIC.h"
#include "IO/FramebufferConsole.h"
#include "Memory/Paging.h"
#include "Memory/PhysicalMemoryManager.h"
#include "Utility/CommandLineExtractor.h"

struct KernelData {
    IO::FramebufferConsole framebufferConsole;
    Utility::CommandLineExtractor commandLineExtractor;
    LogLevel minimumLogLevel;
    Memory::PhysicalMemoryManager physicalMemoryManager;
    Memory::Paging paging{physicalMemoryManager};
    Interrupts::IDT idt{paging};
    Firmware::ACPI acpi{paging};
    Interrupts::IOAPICManager ioapicManager{paging, acpi};
};

struct CpuData {
    uint32_t cpuId;
    Memory::Paging::State *pageState;
    Interrupts::LAPIC lapic;
};

#endif //BOREALOS_KERNELDATA_H
