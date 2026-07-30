#ifndef BOREALOS_LAPIC_H
#define BOREALOS_LAPIC_H

#include <Definitions.h>
#include <Memory/Paging.h>
#include <Firmware/ACPI.h>

#include "IOAPIC.h"

namespace Interrupts {
    namespace LAPICRegister {
        static constexpr uint32_t IOAPIC_VERSION = 0x01;
        static constexpr uint32_t LAPIC_ID = 0x20;
        static constexpr uint32_t MADT_VLREC = 0x2C;
        static constexpr uint32_t ERROR_STATUS = 0x280;
        static constexpr uint32_t SPIRV = 0xF0;
        static constexpr uint32_t EOI = 0xB0;
        static constexpr uint32_t TPR = 0x80;
        static constexpr uint32_t INITIAL_COUNT = 0x380;
        static constexpr uint32_t DIVIDE_CONFIG = 0x3E0;
        static constexpr uint32_t CURRENT_COUNT = 0x390;
        static constexpr uint32_t ICR_LOW = 0x300;
        static constexpr uint32_t ICR_HIGH = 0x310;
    }

    enum class TimerDivide : uint32_t {
        By1 = 0b1011,
        By2 = 0b0000,
        By4 = 0b0001,
        By8 = 0b0010,
        By16 = 0b0011,
        By32 = 0b1000,
        By64 = 0b1001,
        By128 = 0b1010,
    };

    enum class DestinationShorthand : uint8_t {
        NoShorthand = 0,
        Self = 1,
        AllIncludingSelf = 2,
        AllExcludingSelf = 3,
    };

    class LAPIC {
    public:
        static constexpr uint64_t IA32_APIC_MSR = 0x1B;
        static constexpr uint32_t LVT_TIMER_OFFSET = 0x320;
        static constexpr uint32_t LVT_LINT0_OFFSET = 0x350;
        static constexpr uint32_t LVT_LINT1_OFFSET = 0x360;
        static constexpr uint32_t LVT_ERROR_OFFSET = 0x370;
        static constexpr uint32_t SPIRV_VECTOR = 0xFF;
        static constexpr uint32_t LVT_VECTOR = 0x40;
        static constexpr uint8_t MINIMUM_IRQ_NUM = 0x00;
        static constexpr uint8_t IRQ_OFFSET = 0x20;

        LAPIC() = default;
        void Initialize(Memory::Paging *paging, Firmware::ACPI *acpi);
        void SendEOI();

        void SetLvtTimer(uint8_t vector, TimerDivide divide, bool periodic, uint32_t count);
        uint32_t GetLvtCount();

        void SendIPI(uint8_t vector, uint32_t destination, DeliveryMode deliveryMode, TriggerMode triggerMode, DestinationShorthand destinationShorthand);
        void SendIPIAllExcludingSelf(uint8_t vector);
        void SendIPISelf(uint8_t vector);
        uint32_t ReceiveIPI();

    private:
        Memory::Paging *_paging{};
        Firmware::ACPI *_acpi{};
        uint32_t *_mmioBase{};

        static uint32_t *SharedLapicVirtualBase;

        void WriteRegister(uint32_t reg, uint32_t value);
        uint32_t ReadRegister(uint32_t reg);
        void MaskLVTEntry(uint32_t lvtRegister);
        void UnmaskLVTEntry(uint32_t lvtRegister);
    };
} // Interrupts

#endif //BOREALOS_LAPIC_H
