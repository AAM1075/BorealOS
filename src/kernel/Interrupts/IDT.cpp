#include "IDT.h"

#include "Kernel.h"

Interrupts::IDT::IDT(Memory::Paging &paging) : _paging(paging) {

}

static Interrupts::IDT::IDTEntry _idtEntries[256] ALIGNED(16) = {}; // Static allocation for 256 IDT entries

void Interrupts::IDT::Initialize() {
    asm volatile ("cli");

    _idtPointer.Base = reinterpret_cast<uint64_t>(&_idtEntries[0]);
    _idtPointer.Limit = sizeof(IDTEntry) * 256 - 1;

    for (uint16_t vec = 0; vec < 256; vec++) {
        SetIDTEntry(vec, reinterpret_cast<uint64_t>(((void**) &ISRStubTable)[vec]), 0x8E); // 0x8E = Present, DPL=0, Type=Interrupt Gate
    }

    // Override the stack for DF, GPF and PF to use the IST entry 1.
    _idtEntries[8].IST = 1; // Double Fault
    _idtEntries[13].IST = 1; // General Protection Fault
    _idtEntries[14].IST = 1; // Page Fault

    for (auto & _exceptionHandler : _exceptionHandlers) {
        _exceptionHandler = nullptr; // Initialize exception handlers to nullptr
    }

    for (auto & _irqHandler : _irqHandlers) {
        _irqHandler = nullptr; // Initialize IRQ handlers to nullptr
    }

    asm volatile("lidt %0" : : "m" (_idtPointer)); // Load the IDT
    asm volatile("sti"); // Enable interrupts after loading IDT

    _isTesting = true;
    asm volatile("int $0"); // Trigger Division By Zero
    asm volatile("int $3"); // Trigger Breakpoint
    _isTesting = false;

    // Reserve 0-32
    _allocatedVectors[0] = 0xFF;
    _allocatedVectors[1] = 0xFF;
    _allocatedVectors[2] = 0xFF;
    _allocatedVectors[3] = 0xFF;

    // Reserve 253, 254 and 255
    _allocatedVectors[31] = 0xE0; // 11100000
}

void Interrupts::IDT::RegisterExceptionHandler(uint8_t exceptionVector, void(*handler)()) {
    if (exceptionVector >= 32) {
        LOG_ERROR("Attempted to register an exception handler for vector {}, which is not a CPU exception vector!", exceptionVector);
        return;
    }

    _exceptionHandlers[exceptionVector] = handler;
}

void Interrupts::IDT::RegisterIRQHandler(uint8_t irq, void(*handler)()) {
    _irqHandlers[irq] = handler;
}

void Interrupts::IDT::IRQHandler(uint8_t irq, Registers *registers) {
    auto cpuData = Kernel::GetInstance().GetCpuData();
    Memory::Paging::State* backupState = cpuData->pageState; // Backup the current paging state before handling the interrupt
    _paging.SwitchToKernelPageTable();

    _registersForInterrupts[irq] = registers; // Save the register state for this interrupt so that it can be accessed by the handler if needed
    if (_irqHandlers[irq] != nullptr) {
        _irqHandlers[irq]();
    }

    if (backupState != cpuData->pageState) {
        Memory::Paging::SwitchToPageTable(backupState);
    }

    Kernel::GetInstance().GetCpuData()->lapic.SendEOI();
}

void Interrupts::IDT::HandleException(uint32_t exceptionVector, uint32_t errorCode, Registers *registers) const {
    if (_isTesting) {
        LOG_INFO("IDT testing mode: Exception {} occurred with error code {}",
                 (exceptionVector < 32) ? ExceptionNames[exceptionVector] : "Unknown", errorCode);
        return; // In testing mode, just return
    }

    uint64_t cr0, cr2, cr3, cr4;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    asm volatile("mov %%cr2, %0" : "=r"(cr2));
    asm volatile("mov %%cr3, %0" : "=r"(cr3));
    asm volatile("mov %%cr4, %0" : "=r"(cr4));

    LOG_ERROR("\n\r\n\r-- CPU Exception Occurred --\n\r");
    LOG_ERROR("Exception: {} (Vector {})", (exceptionVector < 32) ? ExceptionNames[exceptionVector] : "Unknown", exceptionVector);

    if (exceptionVector == 14) {
        LOG_ERROR("Page fault at address {}", (void*)cr2);
    }

    LOG_ERROR("Error Code {}", (void*)(uint64_t)errorCode);
    LOG_ERROR("RAX: {} RBX: {} RCX: {} RDX: {}", (void*)registers->rax, (void*)registers->rbx, (void*)registers->rcx, (void*)registers->rdx);
    LOG_ERROR("RSI: {} RDI: {} RBP: {} R8: {}", (void*)registers->rsi, (void*)registers->rdi, (void*)registers->rbp, (void*)registers->r8);
    LOG_ERROR("R9: {} R10: {} R11: {} R12: {}", (void*)registers->r9, (void*)registers->r10, (void*)registers->r11, (void*)registers->r12);
    LOG_ERROR("R13: {} R14: {} R15: {}", (void*)registers->r13, (void*)registers->r14, (void*)registers->r15);

    PANIC("Exception occurred");
}

const Interrupts::IDT::Registers * Interrupts::IDT::GetRegistersForInterrupt(uint8_t interruptVector) const {
    return _registersForInterrupts[interruptVector];
}

uint8_t Interrupts::IDT::AllocateVector() {
    for (uint8_t i = 0; i < 32; i++) {
        if (_allocatedVectors[i] != 0xFF) {
            uint8_t bit = __builtin_ffs(~_allocatedVectors[i]) - 1;
            _allocatedVectors[i] |= (1 << bit);
            return (i << 3) | bit;
        }
    }
    return 0;
}

void Interrupts::IDT::FreeVector(uint8_t vector) {
    uint8_t i = vector >> 3;
    uint8_t bit = vector & 7;
    _allocatedVectors[i] &= ~(1 << bit);
}

void Interrupts::IDT::SetIDTEntry(uint8_t vector, uint64_t isr, uint8_t flags) {
    IDTEntry &entry = _idtEntries[vector];
    entry.OffsetLow = isr & 0xFFFF;
    entry.Selector = 0x08; // Kernel code segment
    entry.IST = 0; // No IST
    entry.TypeAttribute = flags;
    entry.OffsetMiddle = (isr >> 16) & 0xFFFF;
    entry.OffsetHigh = (isr >> 32) & 0xFFFFFFFF;
    entry.Zero = 0;
}

extern "C" {
    void IRQHandler(uint8_t irq, uint64_t errorCode, Interrupts::IDT::Registers* regs) {
        Kernel::GetInstance().Data.idt.IRQHandler(irq, regs);
    }

    // Error code is the vector number, so for example, divide by zero is 0, page fault is 14, etc.
    // The error number is the error code pushed by the CPU for exceptions that push an error code.
    // So for vector 8, that would be 0 for double fault, for vector 14, that would be the page fault error code, etc.
    void ExceptionHandler(uint64_t exceptionVector, uint64_t errorCode, Interrupts::IDT::Registers* regs) {
        Kernel::GetInstance().Data.idt.HandleException(exceptionVector, errorCode, regs);
    }
}
