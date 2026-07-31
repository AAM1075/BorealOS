#ifndef BOREALOS_IOAPIC_H
#define BOREALOS_IOAPIC_H

#include <Definitions.h>

#include "Firmware/ACPI.h"
#include "Memory/Paging.h"

namespace Interrupts {
    namespace IOAPICRegister {
        static constexpr uint32_t ID = 0x00;
        static constexpr uint32_t VER = 0x01;
        static constexpr uint32_t ARB = 0x02;
        static constexpr uint32_t RedirectionTableBase = 0x10;
    }

    namespace RedirectionFlags {
        constexpr uint64_t Masked = (1ULL << 16);
    }

    enum class DeliveryMode : uint8_t {
        Fixed = 0b000,
        LowestPriority = 0b001,
        SMI = 0b010,
        NMI = 0b100,
        INIT = 0b101,
        ExtINT = 0b111,
    };

    enum class DestinationMode : uint8_t {
        Physical = 0,
        Logical = 1,
    };

    enum class Polarity : uint8_t {
        ActiveHigh = 0,
        ActiveLow = 1,
    };

    enum class TriggerMode : uint8_t {
        Edge = 0,
        Level = 1,
    };

    struct RedirectionEntry {
        uint8_t vector;
        DeliveryMode deliveryMode = DeliveryMode::Fixed;
        DestinationMode destinationMode = DestinationMode::Physical;
        Polarity polarity = Polarity::ActiveHigh;
        TriggerMode triggerMode = TriggerMode::Edge;
        bool masked = false;
        uint8_t destination;

        uint64_t Pack() const {
            uint64_t entry = vector;
            entry |= (static_cast<uint64_t>(deliveryMode) << 8);
            entry |= (static_cast<uint64_t>(destinationMode) << 11);
            entry |= (static_cast<uint64_t>(polarity) << 13);
            entry |= (static_cast<uint64_t>(triggerMode) << 15);
            entry |= (masked ? (1ULL << 16) : 0);
            entry |= (static_cast<uint64_t>(destination) << 56);
            return entry;
        }
    };

    inline void ApplyMpsInitFlags(RedirectionEntry &entry, uint16_t flags) {
        uint8_t pol = flags & 0x3;
        entry.polarity = (pol == 0b11) ? Polarity::ActiveLow : Polarity::ActiveHigh;
        uint8_t trig = (flags >> 2) & 0x3;
        entry.triggerMode = (trig == 0b11) ? TriggerMode::Level : TriggerMode::Edge;
    }

    class IOAPIC {
    public:
        IOAPIC() = default;

        void Initialize(Memory::Paging *paging, uint32_t *ioApicAddr, uint32_t gsiBase, uint8_t ioApicId);

        bool HasGSI(uint32_t gsi);
        void SetRedirectionEntry(uint32_t gsi, const RedirectionEntry& entry) {
            SetRedirectionEntry(gsi, entry.Pack());
        }
        void SetRedirectionEntry(uint32_t gsi, uint64_t entry);

    private:
        Memory::Paging *_paging{};
        Threading::Spinlock _lock{};
        volatile uint32_t *_mmioBase{};
        uint32_t _gsiBase{};
        uint8_t _ioApicId{};
        uint16_t _version{};
        uint32_t _maxRedirectionEntries{};

        [[nodiscard]] uint32_t ReadRegister(uint32_t reg);
        void WriteRegister(uint32_t reg, uint32_t value);
        void SetRedirectionEntryInternal(uint32_t index, uint64_t entry);
    };

    class IOAPICManager {
    public:
        struct IRQMapping {
            uint32_t gsi;
            uint16_t isoFlags;
        };

        IOAPICManager(Memory::Paging &paging, Firmware::ACPI &acpi);
        void Initialize();

        IOAPIC *GetOwningApic(uint32_t gsi);
        IRQMapping GetIRQ(uint32_t irq);

    private:
        Memory::Paging& _paging;
        Firmware::ACPI& _acpi;

        IOAPIC _ioApic[Architecture::MaxIOAPICs];
        size_t _ioApicCount{};
        IRQMapping _irqToGsi[16]{};

        static void DisablePIC();
    };
} // Interrupts

#endif //BOREALOS_IOAPIC_H
