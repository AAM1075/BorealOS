#include "Kernel.h"
#include "IO/Serial.h"
#include "Parameters.h"
#include <Logging.h>
#include "Interrupts/GDT.h"

void Kernel::Initialize() {
    Data.debugPort.Initialize();
    Data.framebufferConsole.Initialize();
    Data.commandLineExtractor.Initialize();

    auto logLevel = Data.commandLineExtractor.GetValue<uint64_t>(Parameters::LOG_LEVEL);
    if (logLevel.HasValue())
        Data.minimumLogLevel = static_cast<LogLevel>(logLevel.Value());

    auto mp = Boot::Limine::MultiProcessingRequest.response;
    if (!mp)
        PANIC("Limine MultiProcessingRequest response is null, cannot continue!");

    Interrupts::GDT::Initialize(
        mp->bsp_lapic_id,
        (void*)Architecture::StackTop, (void*)Architecture::DefaultFaultHandlerTop
    );

    GetCpuData()->cpuId = mp->bsp_lapic_id;

    Data.physicalMemoryManager.Initialize();
    Data.paging.Initialize();
    Data.idt.Initialize();
    Data.heapAllocator.Initialize();

    Data.cpu.Initialize();
    Core::CPU::InitializeCore(mp->bsp_lapic_id);

    Data.acpi.Initialize();
    Data.ioapicManager.Initialize();
    GetCpuData()->lapic.Initialize(&Data.paging, &Data.acpi);
}

void Kernel::Start() {

}

void Kernel::Log(const char *message, size_t c) {
    Data.debugPort.Write(message, c);
    Data.framebufferConsole.Write(message, c);
}

void Kernel::Panic(const char *message) {
    asm volatile ("cli");

    Data.debugPort.Write("Kernel panic: ", 14);
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

CpuData * Kernel::GetCpuData() {
    return &Cpu[Interrupts::GDT::GetCpuId()];
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

Threading::Spinlock& Logging::GetLogLock() {
    static Threading::Spinlock logLock;
    return logLock;
}

void Core::Write(const char *message, size_t c) {
    Kernel::GetInstance().Log(message, c);
}

[[noreturn]] void Core::Panic(const char *message) {
    Kernel::GetInstance().Panic(message);
}
