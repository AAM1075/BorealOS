#include "LAPIC.h"

#include "Kernel.h"
#include "Cpu.h"

namespace Interrupts {
    void LAPIC::Initialize(Memory::Paging *paging, Firmware::ACPI *acpi) {
        _paging = paging;
        _acpi = acpi;

        uint64_t apicBaseMsr = Cpu::ReadMSR(IA32_APIC_MSR);
        uintptr_t physBase = apicBaseMsr & 0xFFFFF000;

        if (!SharedLapicVirtualBase) {
            SharedLapicVirtualBase = (uint32_t*)_paging->FindAvailableVirtualAddressRangeKernel(1).start;
            _paging->MapPage(Kernel::GetInstance().GetCpuData()->pageState,
                (uintptr_t)SharedLapicVirtualBase,
                physBase,
                Memory::Paging::Flags::Present | Memory::Paging::Flags::ReadWrite
            );
        }

        _mmioBase = SharedLapicVirtualBase;

        if (!(apicBaseMsr & (1 << 11)))
            Cpu::WriteMSR(IA32_APIC_MSR, apicBaseMsr | (1 << 11));

        WriteRegister(LAPICRegister::TPR, 0);
        WriteRegister(LAPICRegister::SPIRV, 0xFF | (1 << 8));

        MaskLVTEntry(LVT_TIMER_OFFSET);
        MaskLVTEntry(LVT_LINT0_OFFSET);
        MaskLVTEntry(LVT_LINT1_OFFSET);
        MaskLVTEntry(LVT_ERROR_OFFSET);

        WriteRegister(LAPICRegister::ERROR_STATUS, 0x00);
        WriteRegister(LAPICRegister::ERROR_STATUS, 0x00);
        WriteRegister(LAPICRegister::EOI, 0x00);

        WriteRegister(LAPICRegister::TPR, MINIMUM_IRQ_NUM);

        asm ("sti");

        LOG_INFO("LAPIC initialized");
    }

    void LAPIC::SendEOI() {
        if (!_mmioBase) return; // This happened before the LAPIC was set up, safe to skip.
        WriteRegister(LAPICRegister::EOI, 0x00);
    }

    void LAPIC::SetLvtTimer(uint8_t vector, TimerDivide divide, bool periodic, uint32_t count) {
        WriteRegister(LAPICRegister::DIVIDE_CONFIG, static_cast<uint32_t>(divide));
        WriteRegister(LVT_TIMER_OFFSET, vector | (periodic ? (1 << 17) : 0));
        WriteRegister(LAPICRegister::INITIAL_COUNT, count);
    }

    uint32_t LAPIC::GetLvtCount() {
        return ReadRegister(LAPICRegister::CURRENT_COUNT);
    }

    void LAPIC::SendIPI(uint8_t vector, uint32_t destination, DeliveryMode deliveryMode, TriggerMode triggerMode,
        DestinationShorthand destinationShorthand) {
        while (ReadRegister(LAPICRegister::ICR_LOW) & (1 << 12)) {
            // wait for any prior IPI to finish sending
        }

        if (destinationShorthand == DestinationShorthand::NoShorthand) {
            WriteRegister(LAPICRegister::ICR_HIGH, destination << 24);
        }

        uint32_t low = vector
            | (static_cast<uint32_t>(deliveryMode) << 8)
            | (1 << 14)
            | (static_cast<uint32_t>(triggerMode) << 15)
            | (static_cast<uint32_t>(destinationShorthand) << 18);

        WriteRegister(LAPICRegister::ICR_LOW, low);

        while (ReadRegister(LAPICRegister::ICR_LOW) & (1 << 12)) {
            // wait for send to complete before returning
        }
    }

    void LAPIC::SendIPIAllExcludingSelf(uint8_t vector) {
        SendIPI(vector, 0, DeliveryMode::Fixed, TriggerMode::Edge, DestinationShorthand::AllExcludingSelf);
    }

    void LAPIC::SendIPISelf(uint8_t vector) {
        SendIPI(vector, 0, DeliveryMode::Fixed, TriggerMode::Edge, DestinationShorthand::Self);
    }

    uint32_t LAPIC::ReceiveIPI() {
        return ReadRegister(LAPICRegister::ICR_LOW) >> 24;
    }

    uint32_t *LAPIC::SharedLapicVirtualBase = nullptr;

    void LAPIC::WriteRegister(uint32_t reg, uint32_t value) {
        _mmioBase[reg / sizeof(uint32_t)] = value;
    }

    uint32_t LAPIC::ReadRegister(uint32_t reg) {
        return _mmioBase[reg / sizeof(uint32_t)];
    }

    void LAPIC::MaskLVTEntry(uint32_t lvtRegister) {
        WriteRegister(lvtRegister, ReadRegister(lvtRegister) | (1 << 16));
    }

    void LAPIC::UnmaskLVTEntry(uint32_t lvtRegister) {
        WriteRegister(lvtRegister, ReadRegister(lvtRegister) & ~(1 << 16));
    }
} // Interrupts