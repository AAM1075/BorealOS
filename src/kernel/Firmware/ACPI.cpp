#include "ACPI.h"

#include "Logging.h"
#include "Boot/LimineDefinitions.h"
#include "IO/Port.h"

namespace Firmware {
    ACPI::ACPI(Memory::Paging &paging) : _paging(paging) {

    }

    void ACPI::Initialize() {
        auto rsdp_request = Boot::Limine::RsdpRequest.response;
        if (!rsdp_request)
            PANIC("Limine RSDP response is null, cannot continue!");

        auto higherHalf = Boot::Limine::HhdmRequest.response->offset; // guaranteed at this point

        _rsdp = static_cast<RSDP *>(rsdp_request->address);
        LOG_DEBUG("RSDP found at {} with revision {} ({})", (void*)_rsdp, _rsdp->revision, SDPRevisionStrings[_rsdp->revision]);

        if (_rsdp->revision > 0) {
            _xsdp = static_cast<XSDP *>(_rsdp);

            if (!ValidateXSDP(_xsdp))
                PANIC("Invalid XSDP checksum, cannot continue!");

            _xsdt = static_cast<XSDT *>((void*)(_xsdp->XSDTAddress + higherHalf));
        } else {
            _rsdt = static_cast<RSDT *>((void*)((uintptr_t)_rsdp->RSDTAddress + higherHalf));

            if (!ValidateSDT(_rsdt))
                PANIC("Invalid RSDT checksum, cannot continue!");
        }

        uint64_t sdtAddrPhys = (_rsdp->revision > 0) ? _xsdp->XSDTAddress : _rsdp->RSDTAddress;
        auto sdt = static_cast<SDTHeader *>((void*)(sdtAddrPhys + higherHalf));

        if (!ValidateSDT(sdt))
            PANIC("Failed to verify SDT");


        _fadt = FindFADT(sdt);
        if (!_fadt)
            PANIC("Failed to find FADT");

        const uint64_t xdsdtOffset = 140; // offsetof doesnt work
        uint64_t dsdtPhys = 0;
        if (_fadt->length >= xdsdtOffset + sizeof(uint64_t) && _fadt->xDsdt != 0) dsdtPhys = _fadt->xDsdt;
        else dsdtPhys = _fadt->dsdt;

        if (!dsdtPhys)
            PANIC("Failed to find DSDT");

        _dsdt = (void*)(dsdtPhys + higherHalf);
        WriteByteCommand(_fadt->acpiEnable);

        if (_fadt->preferredPowerManagementProfile > 7)
            LOG_ERROR("Invalid power profile in FADT");
        else
            LOG_DEBUG("Device has power profile of {}", powerProfileStrings[_fadt->preferredPowerManagementProfile]);

        LOG_INFO("Initialized");
    }

    void * ACPI::GetTable(Utility::StringView signature, size_t index) {
        if (signature == "DSDT") return _dsdt;
        if (signature == "FACP") return _fadt;
        if (signature == "RSDT") return _rsdt;
        if (signature == "XSDT") return _xsdt;

        const auto higherHalf = Boot::Limine::HhdmRequest.response->offset;

        if (_rsdp->revision > 0) {
            size_t entries = (_xsdt->length - sizeof(SDTHeader)) / sizeof(uint64_t);
            for (size_t i = 0; i < entries; i++) {
                uint64_t targetAddr = _xsdt->pointers[i];
                auto table = static_cast<SDTHeader *>((void*)(targetAddr + higherHalf));
                if (Utility::StringView(reinterpret_cast<const char *>(&table->signature[0]), 4) == signature) {
                    if (index == 0)
                        return table;
                    index--;
                }
            }
        } else {
            size_t entries = (_rsdt->length - sizeof(SDTHeader)) / sizeof(uint32_t);
            for (size_t i = 0; i < entries; i++) {
                uint64_t targetAddr = _rsdt->pointers[i];
                auto table = static_cast<SDTHeader *>((void*)(targetAddr + higherHalf));
                if (Utility::StringView(reinterpret_cast<const char *>(&table->signature[0]), 4) == signature) {
                    if (index == 0)
                        return table;
                    index--;
                }
            }
        }

        return nullptr;
    }

    size_t ACPI::FindMADTEntryCount(MADT *madt, MADTEntryType type) {
        auto *current = (Firmware::ACPI::MADTEntryHeader*)((uint8_t*)madt + sizeof(Firmware::ACPI::MADT));
        size_t count = 0;

        while ((uint8_t*)current < (uint8_t*)madt + madt->length) {
            if (current->type == (uint8_t)type) {
                count++;
            }

            current = (Firmware::ACPI::MADTEntryHeader*)((uint8_t*)current + current->length);
        }

        return count;
    }

    ACPI::MADTEntryHeader * ACPI::FindMADTEntry(MADT *madt, MADTEntryType type, size_t index) {
        auto *current = (Firmware::ACPI::MADTEntryHeader*)((uint8_t*)madt + sizeof(Firmware::ACPI::MADT));

        while ((uint8_t*)current < (uint8_t*)madt + madt->length) {
            if (current->type == (uint8_t)type) {
                if (index == 0) {
                    return current;
                }
                index--;
            }

            current = (Firmware::ACPI::MADTEntryHeader*)((uint8_t*)current + current->length);
        }

        return nullptr;
    }

    void ACPI::WriteByteCommand(uint8_t command) {
        Threading::ScopedLock(_lock, true);
        if (!(IO::Port::inw(_fadt->PM1aControlBlock) & 1)) {
            IO::Port::outb(_fadt->SMICommandPort, command);
            while (!(IO::Port::inw(_fadt->PM1aControlBlock) & 1));
        }
    }

    bool ACPI::ValidateXSDP(XSDP *xsdp) {
        if (!xsdp) return false;
        if (!ValidateRSDP(xsdp)) return false;
        if (xsdp->length < sizeof(XSDP)) return false;

        uint8_t sum = 0;
        auto bytes = reinterpret_cast<uint8_t *>(xsdp);

        for (uint32_t i = 0; i < xsdp->length; i++) {
            sum += bytes[i];
        }

        if (sum != 0) return false;
        return true;
    }

    bool ACPI::ValidateRSDP(RSDP *rsdp) {
        if (!rsdp) return false;

        uint8_t sum = 0;
        auto bytes = reinterpret_cast<uint8_t *>(rsdp);

        for (uint32_t i = 0; i < sizeof(RSDP); i++) {
            sum += bytes[i];
        }

        if (sum != 0) return false;
        return true;
    }

    bool ACPI::ValidateSDT(SDTHeader *sdt) {
        if (!sdt) return false;
        if (sdt->length < sizeof(SDTHeader)) return false;

        uint8_t sum = 0;
        for (uint32_t i = 0; i < sdt->length; i++) {
            sum += reinterpret_cast<uint8_t *>(sdt)[i];
        }

        if (sum != 0) return false;
        return true;
    }

    ACPI::FADT * ACPI::FindFADT(SDTHeader *sdt) {
        const auto higherHalf = Boot::Limine::HhdmRequest.response->offset;

        size_t entries = (sdt->length - sizeof(SDTHeader)) / ((_rsdp->revision > 0) ? sizeof(uint64_t) : sizeof(uint32_t));
        for (size_t entry = 0; entry < entries; entry++) {
            uint64_t table = 0;

            if (_rsdp->revision > 0) {
                table = (reinterpret_cast<XSDT *>(sdt))->pointers[entry];
            } else {
                table = (reinterpret_cast<RSDT *>(sdt))->pointers[entry];
            }

            if (Utility::StringView(reinterpret_cast<const char *>(&reinterpret_cast<SDTHeader *>(table + higherHalf)->signature[0]), 4) == "FACP") {
                return reinterpret_cast<FADT *>(table + higherHalf);
            }
        }

        return nullptr;
    }
} // Firmware