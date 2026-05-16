#include <Definitions.h>
#include <Kernel.h>
#include <stdarg.h>

#include "Interrupts/IDT.h"
#include "Syscalls/Syscalls.h"
#include "Utility/HashMap.h"

extern "C" {
    #include <x86_64/dbg.h> // minidbg
}

#include "KernelData.h"
#include "Interrupts/GDT.h"
#include "Interrupts/TSS.h"
#include "IO/Serial.h"
#include "IO/SerialPort.h"
#include "IO/FramebufferConsole.h"
#include "Utility/StringFormatter.h"
#include "Utility/ANSI.h"
#include "Memory/PMM.h"
#include "Interrupts/Syscall.h"

Kernel<KernelData> kernel;
KernelData kernelData;
bool debugLogging = false;
bool enableTicking = false;

void apic_timer_handler() {
    kernelData.Hpet.Tick(); // To make sure the HPET's internal tick count does not overflow.

    // We send the EOI *before* actually finishing this, since the Tick might enter userspace or do something else, which means the APIC interrupt wont resend.
    kernelData.Apic->SendEOI(Interrupts::APIC::LVT_VECTOR);

    if (kernelData.DefaultScheduler && enableTicking)
        kernelData.DefaultScheduler->Tick((void*)kernelData.Idt.GetRegistersForInterrupt(Interrupts::APIC::LVT_VECTOR - 0x20));
}

template<typename T>
Kernel<T> *Kernel<T>::GetInstance() {
    return &kernel;
}

template<typename T>
void Kernel<T>::Initialize() {
    ArchitectureData = &kernelData;

    // Serial:
    ArchitectureData->SerialPort = IO::SerialPort(IO::Serial::COM1);
    ArchitectureData->SerialPort.Initialize();
    ArchitectureData->SerialPort.WriteString("\n\n");
    LOG(LOG_LEVEL::INFO, "Loaded serial port COM1 (%p).", IO::Serial::COM1);

    // Tss & Gdt:
    Interrupts::GDT::Initialize(); // This loads the GDT and the TSS into the GDT
    LOG(LOG_LEVEL::INFO, "Initialized GDT & TSS. (GDT at %p, TSS at %p)",
        Interrupts::GDT::GetGDTPointer(),
        Interrupts::TSS::GetTSSStruct());

    // Syscall:
    Interrupts::Syscall::Initialize();
    Syscalls::Initialize(); // Load the syscall handlers into the syscall handler array.
    LOG(LOG_LEVEL::INFO, "Initialized syscall handling.");

    // PIC:
    static uint8_t picData [sizeof(Interrupts::PIC)]; // bit ugly, but required to init a vtable class before we can use new to allocate it on the heap.
    auto pic = new (picData) Interrupts::PIC(0x20, 0x28);
    ArchitectureData->Pic = pic;
    ArchitectureData->Pic->Initialize();
    LOG(LOG_LEVEL::INFO, "Initialized PIC.");

    // IDT:
    ArchitectureData->Idt = Interrupts::IDT(ArchitectureData->Pic);
    ArchitectureData->Idt.Initialize();
    LOG(LOG_LEVEL::INFO, "Initialized IDT.");

    // Console:
    ArchitectureData->Console.Initialize();
    LOG(LOG_LEVEL::INFO, "Initialized framebuffer console.");

    // RTC:
    ArchitectureData->Rtc = Core::Time::RTC(&ArchitectureData->Idt);
    ArchitectureData->Rtc.Initialize();
    LOG(LOG_LEVEL::INFO, "Initialized RTC.");

    // Physical Memory Manager:
    ArchitectureData->Pmm.Initialize();
    LOG(LOG_LEVEL::INFO, "Initialized PMM.");

    // CPU:
    ArchitectureData->Cpu.Initialize();
    LOG(LOG_LEVEL::INFO, "Initialized CPU.");

    // Paging:
    ArchitectureData->Paging = Memory::Paging(&ArchitectureData->Pmm);
    ArchitectureData->Paging.Initialize();
    ArchitectureData->Idt.SetPagingManager(&ArchitectureData->Paging); // We need to set the paging reference in the IDT so that the IDT can switch back to the kernel's page table if an interrupt occurs while we're running with a different page table.
    LOG(LOG_LEVEL::INFO, "Initialized paging.");

    // Heap:
    ArchitectureData->HeapAllocator = Memory::HeapAllocator(&ArchitectureData->Pmm, &ArchitectureData->Paging, ArchitectureData->Paging.GetKernelPagingState());
    ArchitectureData->HeapAllocator.Initialize();
    LOG(LOG_LEVEL::INFO, "Initialized heap allocator.");

    // ACPI:
    ArchitectureData->Acpi.Initialize();
    LOG(LOG_LEVEL::INFO, "Initialized ACPI.");

    // HPET:
    ArchitectureData->Hpet = Core::Time::HPET(&ArchitectureData->Acpi, &ArchitectureData->Paging, &ArchitectureData->Idt);
    ArchitectureData->Hpet.Initialize();
    LOG(LOG_LEVEL::INFO, "Initialized HPET.");

    // APIC:
    ArchitectureData->Apic = new Interrupts::APIC(&ArchitectureData->Acpi, &ArchitectureData->Cpu, ArchitectureData->Pic, &ArchitectureData->Paging, &ArchitectureData->Idt, &ArchitectureData->Hpet);
    ArchitectureData->Apic->Initialize();
    LOG(LOG_LEVEL::INFO, "Initialized APIC.");

    ArchitectureData->Idt.RegisterIRQHandler(Interrupts::APIC::LVT_VECTOR - 0x20, apic_timer_handler);

    // Invariant TSC:
    ArchitectureData->Tsc = Core::Time::TSC(&ArchitectureData->Hpet, &ArchitectureData->Cpu);
    LOG(LOG_LEVEL::INFO, "Initialized TSC with frequency approximately %u64hz.", ArchitectureData->Tsc.GetFrequency());

    // PCI:
    ArchitectureData->Pci = new IO::PCI(&ArchitectureData->Paging);
    ArchitectureData->Pci->Initialize();
    LOG(LOG_LEVEL::INFO, "Initialized PCI.");

    // Init ram fs:
    auto files = module_request.response->modules;
    if (!files || module_request.response->module_count == 0) {
        PANIC("Limine did not provide any modules, but we need at least one for the init ram filesystem!");
    }

    auto cpioArchive = files[0];
    ArchitectureData->InitRamFS = new FileSystem::InitRam(cpioArchive, &ArchitectureData->HeapAllocator);
    LOG_INFO("Initialized initramfs.");

    // VFS:
    ArchitectureData->VFS = new FileSystem::VirtualFileSystem();
    if (ArchitectureData->VFS->Mount("initramfs", ArchitectureData->InitRamFS) != FileSystem::FSResult::SUCCESS) {
        PANIC("Failed to mount init ram filesystem to virtual filesystem!");
    }

    LOG_INFO("Mounted initramfs to virtual filesystem with prefix \"initramfs:/\".");

    // Kernel symbols:
    auto symbolTable = ArchitectureData->InitRamFS->OpenOrPanic("/ramfs/kernel.sym");
    FileSystem::FileInfo symbolTableInfo = ArchitectureData->InitRamFS->GetFileInfoOrPanic(symbolTable);

    auto symbolTableData = new uint8_t[symbolTableInfo.size];
    ArchitectureData->InitRamFS->ReadOrPanic(symbolTable, symbolTableData, symbolTableInfo.size);
    ArchitectureData->KernelSymbols = new Formats::SymbolLoader(symbolTableData, symbolTableInfo.size);
    LOG_INFO("Initialized kernel symbol loader with %u64 symbols.", ArchitectureData->KernelSymbols->GetSymbolCount());

    // Service manager:
    ArchitectureData->ServiceManager = new Core::ServiceManager();

    // Driver manager:
    ArchitectureData->DriverManager = new Core::Drivers::DriverManager("/ramfs/modules", ArchitectureData->KernelSymbols, ArchitectureData->InitRamFS, &ArchitectureData->Paging, &ArchitectureData->Pmm);
    LOG_INFO("Initialized driver manager.");

    ArchitectureData->Stdio = new FileSystem::STDIO(&ArchitectureData->HeapAllocator);

    // Scheduler:
    ArchitectureData->DefaultScheduler = new Core::Time::Scheduler(&ArchitectureData->Tsc);
    LOG_INFO("Initialized scheduler.");

    // Load the AML interpreter:
    ArchitectureData->Acpi.LoadLAI();
    LOG_INFO("Initialized ACPI AML interpreter (LAI).");

    ArchitectureData->ProcessManager = new Core::ProcessManager();
    ArchitectureData->ThreadScheduler = new Core::ThreadScheduler(ArchitectureData->ProcessManager, ArchitectureData->DefaultScheduler);
}

template<typename T>
void Kernel<T>::Start() {
    // Load all drivers, we do this in the start function because this ensures we have finished initialization of all main kernel subsystems before we start loading drivers, which may depend on those subsystems.
    ArchitectureData->DriverManager->LoadDriversFromFileSystem();
    LOG_INFO("Finished loading drivers.");

    FileSystem::VirtualFileSystem *vfs = ArchitectureData->VFS;
    FileSystem::Descriptor initProcessDescriptor{};
    auto result = vfs->Open("initramfs:/ramfs/bin/init", FileSystem::OpenFlags::Read, &initProcessDescriptor);
    if (result != FileSystem::FSResult::SUCCESS) {
        LOG_ERROR("Failed to open init process from virtual filesystem. Error code: %u32", static_cast<uint32_t>(result));
    }

    FileSystem::FileInfo initProcessInfo{};
    result = vfs->GetFileInfo(&initProcessDescriptor, &initProcessInfo);
    if (result != FileSystem::FSResult::SUCCESS) {
        LOG_ERROR("Failed to get file info for init process from virtual filesystem. Error code: %u32", static_cast<uint32_t>(result));
    }

    auto initProcessData = new uint8_t[initProcessInfo.size];
    if (vfs->Read(&initProcessDescriptor, initProcessData, initProcessInfo.size, nullptr) != FileSystem::FSResult::SUCCESS) {
        PANIC("Failed to read init process from virtual filesystem!");
    }

    Core::ProcessManager* pm = ArchitectureData->ProcessManager;
    Core::ThreadScheduler* ts = ArchitectureData->ThreadScheduler;

    auto process = pm->CreateProcess(initProcessData, initProcessInfo.size);
    if (!process) {
        PANIC("Failed to create init process!");
    }

    ts->ScheduleProcess(process, {
        .argv = nullptr,
        .argc = 0,
        .currentWorkingDirectory = "/",
    });

    LOG_INFO("Created and scheduled init process with PID %u64.", process->pid);

    LOG_INFO("Entering main kernel loop. Should be the last log message you see before the init process starts running.");
    enableTicking = true;

    while(true)
        asm("hlt");
}

template<typename T>
void Kernel<T>::Log(const char *message) {
    ArchitectureData->SerialPort.WriteString(message);
}

template<typename T>
[[noreturn]] void Kernel<T>::Panic(const char *message) {
    Log("[PANIC] ");
    Log(message);
    kernelData.Console.PrintString("[");
    kernelData.Console.PrintString(ANSI::Colors::Foreground::Red);
    kernelData.Console.PrintString(ANSI::EscapeCodes::TextDim);
    kernelData.Console.PrintString("PANIC\033[0m] ");
    kernelData.Console.PrintString(message);
    kernelData.Console.PrintString("\n\r");

    // Drop into minidbg
    kernelData.Console.PrintString("[");
    kernelData.Console.PrintString(ANSI::Colors::Foreground::Cyan);
    kernelData.Console.PrintString("DEBUG\033[0m] Dropping into kernel debugger (minidbg)...\n\r");
    dbg_main(1);

    // The debugger can return control, so we need to halt the CPU
    kernelData.Console.PrintString("[");
    kernelData.Console.PrintString(ANSI::Colors::Foreground::Cyan);
    kernelData.Console.PrintString("DEBUG\033[0m] Kernel debugger (minidbg) exited, halting now.\n\r");
    while (true) {
        asm ("hlt");
    }
}

void Core::Write(const char *message) {
    Kernel<KernelData>::GetInstance()->Log(message);
}

void Core::Log(LOG_LEVEL level, const char *fmt, ...) {
    kernelData.Console.PrintString("[");

    switch (level) {
        case LOG_LEVEL::INFO:
            kernel.Log("[INFO] ");
            kernelData.Console.PrintString(ANSI::Colors::Foreground::Green);
            kernelData.Console.PrintString("INFO");
            break;
        case LOG_LEVEL::WARNING:
            kernel.Log("[WARNING] ");
            kernelData.Console.PrintString(ANSI::Colors::Foreground::Yellow);
            kernelData.Console.PrintString("WARNING");
            break;
        case LOG_LEVEL::ERROR:
            kernel.Log("[ERROR] ");
            kernelData.Console.PrintString(ANSI::Colors::Foreground::Red);
            kernelData.Console.PrintString("ERROR");
            break;
        case LOG_LEVEL::DEBUG:
            kernel.Log("[DEBUG] ");
            kernelData.Console.PrintString(ANSI::Colors::Foreground::Cyan);
            kernelData.Console.PrintString("DEBUG");
            break;
    }

    kernelData.Console.PrintString("\033[0m] ");

    // Format the message
    char buffer[1025]; // +1 for null terminator
    va_list args;
    va_start(args, fmt);
    auto len = Utility::StringFormatter::vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    bool truncated = false;
    if (len > 1024) {
        len = 1024;
        truncated = true;
    }
    buffer[len] = '\0';

    kernel.Log(buffer);
    kernelData.Console.PrintString(buffer);
    kernelData.Console.PrintString("\r");
    if (truncated) {
        kernel.Log("...[TRUNCATED]");
        kernelData.Console.PrintString("...[TRUNCATED]\r");
    }
}

[[noreturn]] void Core::Panic(const char *message) {
    asm volatile ("cli"); // Disable interrupts to prevent any further damage or interference with the panic process.
    kernel.Panic(message);
}

extern "C" void kernel_assert(bool condition, const char* message) {
    if (!condition) {
        LOG_ERROR("Assertion failed: %s", message);
        Core::Panic("Assertion failed!");
    }
}

// Unfortunately printf is a requirement for the tlsf allocator.
extern "C" size_t printf(const char *fmt, ...) {
    char buffer[1025]; // +1 for null terminator
    va_list args;
    va_start(args, fmt);
    auto len = Utility::StringFormatter::vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    bool truncated = false;
    if (len > 1024) {
        len = 1024;
        truncated = true;
    }
    buffer[len] = '\0';

    kernel.Log(buffer);
    kernelData.Console.PrintString(buffer);
    kernelData.Console.PrintString("\r");
    if (truncated) {
        kernel.Log("...[TRUNCATED]");
        kernelData.Console.PrintString("...[TRUNCATED]\r");
    }

    return len;
}

template class Kernel<KernelData>; // Initialize the template class