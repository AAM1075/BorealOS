#ifndef BOREALOS_ACPI_H
#define BOREALOS_ACPI_H

#include <Definitions.h>
#include <Utility/StringView.h>

#include "Memory/Paging.h"

namespace Firmware {
    class ACPI {
    private:
        static constexpr Utility::StringView SDPRevisionStrings[] = {
            "ACPI 1.0",
            "Unknown",
            "ACPI 2.0+"
        };

        static constexpr Utility::StringView powerProfileStrings[] = {
            "Unspecified",
            "Desktop",
            "Mobile",
            "Workstation",
            "Enterprise Server",
            "SOHO Server",
            "Appliance PC",
            "Performance Server"
        };

    public:
        struct RSDP {
            uint8_t signature[8];
            uint8_t checksum;
            uint8_t OEMId[6];
            uint8_t revision;
            uint32_t RSDTAddress;
        } PACKED;

        struct XSDP : RSDP {
            uint32_t length;
            uint64_t XSDTAddress;
            uint8_t extendedChecksum;
            uint8_t reserved[3];
        } PACKED;

        struct SDTHeader {
            uint8_t signature[4];
            uint32_t length;
            uint8_t revision;
            uint8_t checksum;
            uint8_t OEMId[6];
            uint8_t OEMTableId[8];
            uint32_t OEMRevision;
            uint32_t creatorId;
            uint32_t creatorRevision;
        } PACKED;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
        struct RSDT : SDTHeader {
            uint32_t pointers[];
        } PACKED;

        struct XSDT : SDTHeader {
            uint64_t pointers[];
        } PACKED;
#pragma GCC diagnostic pop

        struct MADT : SDTHeader {
            uint32_t localAPICAddr;
            uint32_t flags;
        } PACKED;

        struct MADTEntryHeader {
            uint8_t type;
            uint8_t length;
        } PACKED;

        enum class MADTEntryType : uint8_t {
            LocalAPIC = 0,
            IOAPIC = 1,
            IRQSrcOverride = 2,
            NMI = 3,
            LocalAPICNMI = 4,
            LocalAPICOverride = 5,
        };

        struct MADTLocalAPIC : MADTEntryHeader { // type 0
            uint8_t acpiProcessorId;
            uint8_t apicId;
            uint32_t flags;
        } PACKED;

        struct MADTIOAPIC : MADTEntryHeader { // type 1
            uint8_t ioApicId;
            uint8_t reserved;
            uint32_t ioApicAddress;
            uint32_t globalSystemInterruptBase;
        } PACKED;

        struct MADTIRQSrcOverride : MADTEntryHeader { // type 2
            uint8_t busSource;
            uint8_t IRQSource;
            uint32_t globalSysInterrupt;
            uint16_t flags;
        } PACKED;

        struct MADTLocalAPICOverride : MADTEntryHeader { // type 5
            uint16_t reserved;
            uint64_t address;
        } PACKED;

        struct GenericAddr {
            uint8_t addressSpace;
            uint8_t bitWidth;
            uint8_t bitOffset;
            uint8_t accessSize;
            uint64_t address;
        } PACKED;

        struct FADT : SDTHeader {
            uint32_t firmwareControl;
            uint32_t dsdt;
            uint8_t reserved;
            uint8_t preferredPowerManagementProfile;
            uint16_t SCIInterrupt;
            uint32_t SMICommandPort;
            uint8_t acpiEnable;
            uint8_t acpiDisable;
            uint8_t S4BIOSRequired;
            uint8_t PStateControl;
            uint32_t PM1aEventBlock;
            uint32_t PM1bEventBlock;
            uint32_t PM1aControlBlock;
            uint32_t PM1bControlBlock;
            uint32_t PM2ControlBlock;
            uint32_t PMTimerBlock;
            uint32_t GPE0Block;
            uint32_t GPE1Block;
            uint8_t PM1EventLength;
            uint8_t PM1ControlLength;
            uint8_t PM2ControlLength;
            uint8_t PMTimerLength;
            uint8_t GPE0Length;
            uint8_t GPE1Length;
            uint8_t GPE1Base;
            uint8_t CStateControl;
            uint16_t worstC2Latency;
            uint16_t worstC3Latency;
            uint16_t flushSize;
            uint16_t flushStride;
            uint8_t dutyOffset;
            uint8_t dutyWidth;
            uint8_t dayAlarm;
            uint8_t monthAlarm;
            uint8_t century;
            uint16_t bootArchitectureFlags;
            uint8_t reserved2;
            uint32_t flags;
            GenericAddr resetReg;
            uint8_t resetValue;
            uint8_t reserved3[3];
            uint64_t xFirmwareControl;
            uint64_t xDsdt;
            GenericAddr xPM1aEventBlock;
            GenericAddr xPM1bEventBlock;
            GenericAddr xPM1aControlBlock;
            GenericAddr xPM1bControlBlock;
            GenericAddr xPM2ControlBlock;
            GenericAddr xPMTimerBlock;
            GenericAddr xGPE0Block;
            GenericAddr xGPE1Block;
        } PACKED;

        ACPI(Memory::Paging& paging);

        void Initialize();
        void *GetTable(Utility::StringView signature, size_t index);

        static size_t FindMADTEntryCount(MADT *madt, MADTEntryType type);
        static MADTEntryHeader* FindMADTEntry(MADT *madt, MADTEntryType type, size_t index);
    private:
        Memory::Paging &_paging;
        Threading::Spinlock _lock{};
        RSDP *_rsdp{};
        RSDT *_rsdt{};
        XSDP *_xsdp{};
        XSDT *_xsdt{};
        FADT *_fadt{};
        void *_dsdt{};

        FADT *FindFADT(SDTHeader * sdt);
        void WriteByteCommand(uint8_t command);
        static bool ValidateXSDP(XSDP *xsdp);
        static bool ValidateRSDP(RSDP *rsdp);
        static bool ValidateSDT(SDTHeader *sdt);
    };
} // Firmware

#endif //BOREALOS_ACPI_H
