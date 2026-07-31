#include "IOAPIC.h"

#include "Kernel.h"
#include "Logging.h"
#include "IO/PortIO.h"

namespace {
    constexpr int PIC1 = 0x20;
    constexpr int PIC2 = 0xA0;
    constexpr int PIC1_DATA = PIC1 + 1;
    constexpr int PIC2_DATA = PIC2 + 1;
}

namespace Interrupts {
    void IOAPIC::Initialize(Memory::Paging *paging, uint32_t *ioApicAddr, uint32_t gsiBase, uint8_t ioApicId) {
        _paging = paging;
        _mmioBase = ioApicAddr;
        _gsiBase = gsiBase;
        _ioApicId = ioApicId;

        if (!_mmioBase)
            PANIC("MMIO Base address is null");

        uint32_t versionReg = ReadRegister(IOAPICRegister::VER);
        _version = versionReg & 0xFF;
        _maxRedirectionEntries = ((versionReg >> 16) & 0xFF) + 1;

        for (uint32_t i = 0; i < _maxRedirectionEntries; i++) {
            SetRedirectionEntryInternal(i, RedirectionFlags::Masked);
        }

        LOG_INFO("Initialized IOAPIC {} at GSI base {} with version {} and {} redirection entries", _ioApicId, _gsiBase, _version, _maxRedirectionEntries);
    }

    bool IOAPIC::HasGSI(uint32_t gsi) {
        return (gsi >= _gsiBase) && (gsi < (_gsiBase + _maxRedirectionEntries));
    }

    void IOAPIC::SetRedirectionEntry(uint32_t gsi, uint64_t entry) {
        if (!HasGSI(gsi)) {
            LOG_ERROR("Attempted to set redirection entry for GSI {} which is out of bounds for IOAPIC {} with {} entries", gsi, _ioApicId, _maxRedirectionEntries);
            return;
        }
        SetRedirectionEntryInternal(gsi - _gsiBase, entry);
    }

    uint32_t IOAPIC::ReadRegister(uint32_t reg) {
        Threading::ScopedLock lock(_lock, true);
        _mmioBase[0] = (reg & 0xFF);
        return _mmioBase[4];
    }

    void IOAPIC::WriteRegister(uint32_t reg, uint32_t value) {
        Threading::ScopedLock lock(_lock, true);
        _mmioBase[0] = (reg & 0xFF);
        _mmioBase[4] = value;
    }

    void IOAPIC::SetRedirectionEntryInternal(uint32_t index, uint64_t entry) {
        if (index >= _maxRedirectionEntries) {
            LOG_ERROR("Attempted to set redirection entry {} which is out of bounds for IOAPIC {} with {} entries", index, _ioApicId, _maxRedirectionEntries);
            return;
        }

        uint8_t regLow = IOAPICRegister::RedirectionTableBase + (index * 2);
        uint8_t regHigh = regLow + 1;

        WriteRegister(regHigh, (entry >> 32) & 0xFFFFFFFF);
        WriteRegister(regLow, entry & 0xFFFFFFFF);
    }
} // IOAPIC

namespace Interrupts {
    IOAPICManager::IOAPICManager(Memory::Paging &paging, Firmware::ACPI &acpi) : _paging(paging), _acpi(acpi) {

    }

    void IOAPICManager::Initialize() {
        DisablePIC();

        auto madt = (Firmware::ACPI::MADT*)_acpi.GetTable("APIC", 0);

        for (uint32_t i = 0; i < 16; i++) {
            _irqToGsi[i] = {i, 0};
        }

        size_t isoCount = Firmware::ACPI::FindMADTEntryCount(madt, Firmware::ACPI::MADTEntryType::IRQSrcOverride);
        for (size_t i = 0; i < isoCount; i++) {
            auto entry = (Firmware::ACPI::MADTIRQSrcOverride*)Firmware::ACPI::FindMADTEntry(madt, Firmware::ACPI::MADTEntryType::IRQSrcOverride, i);
            if (!entry) continue;
            if (entry->IRQSource < 16) {
                _irqToGsi[entry->IRQSource] = {entry->globalSysInterrupt, entry->flags};
            }
        }

        _ioApicCount = Firmware::ACPI::FindMADTEntryCount(madt, Firmware::ACPI::MADTEntryType::IOAPIC);
        if (_ioApicCount > Architecture::MaxIOAPICs) {
            LOG_ERROR("Found {} IOAPICs, but the maximum supported is {}", _ioApicCount, Architecture::MaxIOAPICs);
            _ioApicCount = Architecture::MaxIOAPICs;
        }

        for (size_t i = 0; i < _ioApicCount; i++) {
            auto entry = (Firmware::ACPI::MADTIOAPIC*)Firmware::ACPI::FindMADTEntry(madt, Firmware::ACPI::MADTEntryType::IOAPIC, i);
            if (!entry) {
                LOG_ERROR("Failed to find IOAPIC entry {}", i);
                continue;
            }

            uintptr_t ioApicMmio = _paging.FindAvailableVirtualAddressRangeKernel(1).start;
            _paging.MapPage(Kernel::GetInstance().GetCpuData()->pageState,
                ioApicMmio,
                entry->ioApicAddress,
                Memory::Paging::Flags::Present | Memory::Paging::Flags::ReadWrite | Memory::Paging::Flags::CacheDisable
            );

            _ioApic[i].Initialize(&_paging, (uint32_t*)ioApicMmio, entry->globalSystemInterruptBase, entry->ioApicId);
        }

        LOG_INFO("Initialized IOAPIC with {} IOAPIC(s)", _ioApicCount);
    }

    IOAPIC * IOAPICManager::GetOwningApic(uint32_t gsi) {
        for (size_t i = 0; i < _ioApicCount; i++) {
            if (_ioApic[i].HasGSI(gsi)) {
                return &_ioApic[i];
            }
        }

        return nullptr;
    }

    IOAPICManager::IRQMapping IOAPICManager::GetIRQ(uint32_t irq) {
        return _irqToGsi[irq];
    }

    void IOAPICManager::DisablePIC() {
        IO::Port::outb(PIC1_DATA, 0xFF);
        IO::Port::IOWait();
        IO::Port::outb(PIC2_DATA, 0xFF);
        IO::Port::IOWait();
    }
} // IOAPICManager