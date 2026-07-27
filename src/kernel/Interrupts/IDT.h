#ifndef BOREALOS_IDT_H
#define BOREALOS_IDT_H

#include <Definitions.h>

#include "../Memory/Paging.h"

static const char* ExceptionNames[] = {
    "<FATAL_CPU_EXCEPTION_0> Division By Zero",
    "<FATAL_CPU_EXCEPTION_1> Debug",
    "<FATAL_CPU_EXCEPTION_2> Non Maskable Interrupt",
    "<FATAL_CPU_EXCEPTION_3> Breakpoint",
    "<FATAL_CPU_EXCEPTION_4> Detected Overflow",
    "<FATAL_CPU_EXCEPTION_5> Out of Bounds",
    "<FATAL_CPU_EXCEPTION_6> Invalid Opcode",
    "<FATAL_CPU_EXCEPTION_7> No Coprocessor",
    "<FATAL_CPU_EXCEPTION_8> Double Fault",
    "<FATAL_CPU_EXCEPTION_9> Coprocessor Segment Overrun",
    "<FATAL_CPU_EXCEPTION_10> Bad TSS",
    "<FATAL_CPU_EXCEPTION_11> Segment Not Present",
    "<FATAL_CPU_EXCEPTION_12> Stack Fault",
    "<FATAL_CPU_EXCEPTION_13> General Protection Fault",
    "<FATAL_CPU_EXCEPTION_14> Page Fault",
    "<FATAL_CPU_EXCEPTION_15> Unknown Interrupt",
    "<FATAL_CPU_EXCEPTION_16> Coprocessor Fault",
    "<FATAL_CPU_EXCEPTION_17> Alignment Check failure (486+)",
    "<FATAL_CPU_EXCEPTION_18> Machine Check failure (Pentium/586+)",
    "<FATAL_CPU_EXCEPTION_19> Reserved",
    "<FATAL_CPU_EXCEPTION_20> Reserved",
    "<FATAL_CPU_EXCEPTION_21> Reserved",
    "<FATAL_CPU_EXCEPTION_22> Reserved",
    "<FATAL_CPU_EXCEPTION_23> Reserved",
    "<FATAL_CPU_EXCEPTION_24> Reserved",
    "<FATAL_CPU_EXCEPTION_25> Reserved",
    "<FATAL_CPU_EXCEPTION_26> Reserved",
    "<FATAL_CPU_EXCEPTION_27> Reserved",
    "<FATAL_CPU_EXCEPTION_28> Reserved",
    "<FATAL_CPU_EXCEPTION_29> Reserved",
    "<FATAL_CPU_EXCEPTION_30> Reserved",
    "<FATAL_CPU_EXCEPTION_31> Reserved",
};

extern "C" {
    extern void* ISRStubTable[];
}

namespace Interrupts {
    class IDT {
    public:
        struct PACKED IDTEntry {
            uint16_t OffsetLow;
            uint16_t Selector;
            uint8_t IST;
            uint8_t TypeAttribute;
            uint16_t OffsetMiddle;
            uint32_t OffsetHigh;
            uint32_t Zero;
        };

        struct PACKED IDTPointer {
            uint16_t Limit;
            uint64_t Base;
        };

#pragma pack(push, 1)
        struct Registers {
            uint64_t rax, rbx, rcx, rdx, rsi, rdi, rbp, r8, r9, r10, r11, r12, r13, r14, r15;
            uint64_t error_code;
            uint64_t rip;
            uint64_t cs;
            uint64_t rflags;
            uint64_t user_rsp; /* The RSP of the process that was interrupted */
            uint64_t ss;
        };
#pragma pack(pop)

        explicit IDT(Memory::Paging& paging);

        void Initialize();
        void RegisterExceptionHandler(uint8_t exceptionVector, void (*handler)());
        void RegisterIRQHandler(uint8_t irq, void (*handler)());
        void IRQHandler(uint8_t irq, Registers *registers);
        void HandleException(uint32_t exceptionVector, uint32_t errorCode, Registers *registers) const;
        void UnmaskIRQ(uint8_t uint8) const;
        void MaskIRQ(uint8_t uint8) const;
        [[nodiscard]] const Registers *GetRegistersForInterrupt(uint8_t interruptVector) const;

    private:
        IDTPointer _idtPointer = {0, 0};
        void (*_exceptionHandlers[32])() = { nullptr };
        void (*_irqHandlers[256])() = { nullptr };
        Registers* _registersForInterrupts[256] = {}; // This is used to store the register state for the currently handled interrupt, so that it can be accessed by the exception handler (for exceptions) or the IRQ handler (for IRQs) if needed. It is indexed by the interrupt vector number.
        void SetIDTEntry(uint8_t vector, uint64_t isr, uint8_t flags);
        bool _isTesting = false;
        Memory::Paging& _paging;
    };
}

#endif //BOREALOS_IDT_H
