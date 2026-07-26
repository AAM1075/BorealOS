#include "Kernel.h"
#include "Parameters.h"
#include <Logging.h>
#include "Interrupts/GDT.h"

void Kernel::Initialize() {
    Data.framebufferConsole.Initialize();
    Data.commandLineExtractor.Initialize();

    auto logLevel = Data.commandLineExtractor.GetValue<uint64_t>(Parameters::LOG_LEVEL);
    if (logLevel.HasValue())
        Data.minimumLogLevel = static_cast<LogLevel>(logLevel.Value());

    auto mp = Boot::Limine::MultiProcessingRequest.response;
    if (!mp)
        PANIC("Limine MultiProcessingRequest response is null, cannot continue!");

    auto cpuId = mp->bsp_lapic_id; // Our core "zero", the core used to boot.

    Interrupts::GDT::Initialize(cpuId, (void*)Architecture::StackTop, (void*)Architecture::DefaultFaultHandlerTop);

    Data.physicalMemoryManager.Initialize();
    Data.paging.Initialize();
    Data.idt.Initialize();
}

void Kernel::Start() {

}

void Kernel::Log(const char *message, size_t c) {
    Data.framebufferConsole.Write(message, c);
}

void Kernel::Panic(const char *message) {
    asm volatile ("cli");

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
