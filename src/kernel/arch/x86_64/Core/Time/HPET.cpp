#include "HPET.h"

#include "Kernel.h"
#include "../../KernelData.h"

namespace Core::Time {
    HPET::HPET(Firmware::ACPI *acpi, Memory::Paging *paging, Interrupts::IDT* idt) : _acpi(acpi), _paging(paging), _idt(idt), _totalTicks(0) {
        _hpetTable = reinterpret_cast<HPETTable*>(_acpi->GetTable("HPET"));
    }

    void HPET::Initialize() {
        if (!_hpetTable) {
            PANIC("No HPET table found in ACPI, cannot initialize HPET! (Are you running on a system pre 2005? or a virtual machine that doesn't support HPET?)");
        }

        uint64_t physicalAddr = _hpetTable->Address.Address;
        uint64_t mmioAddr = Memory::Paging::NextMMIOAddress();

        // Map the HPET MMIO region
        _paging->MapPage(
            mmioAddr,
            physicalAddr,
            Memory::PageFlags::ReadWrite | Memory::PageFlags::NoExecute | Memory::PageFlags::CacheDisable
        );

        _hpetMappedAddress = reinterpret_cast<void*>(mmioAddr);
        volatile auto hpetRegs = static_cast<HPETRegisters*>(_hpetMappedAddress);
        volatile auto capabilities = reinterpret_cast<HPETCapabilities*>(&hpetRegs->GeneralCapabilitiesID);

        _hpetFrequency = FemtoSecond / capabilities->CounterClockPeriod;
        _timerCount = capabilities->NumTimersCap + 1;

        LOG_DEBUG("HPET capabilities:");
        LOG_DEBUG("  Counter Clock Period: %u64hz", _hpetFrequency);
        LOG_DEBUG("  Number of Timers: %u64", _timerCount);
        LOG_DEBUG("  Counter Size: %s", capabilities->CountSizeCap ? "64-bit" : "32-bit");

        asm volatile ("cli"); // Disable interrupts while configuring HPET

        // Disable HPET before configuring it
        volatile auto configReg = reinterpret_cast<uint64_t *>(static_cast<char *>(_hpetMappedAddress) + 0x10);
        *configReg = 0;

        // Reset the main counter.
        volatile auto mainCounter = reinterpret_cast<uint64_t*>(static_cast<char*>(_hpetMappedAddress) + 0xF0);
        *mainCounter = 0;

        // Enable HPET
        SET_BIT(*configReg, 0);

        asm volatile ("sti");
    }

    uint64_t HPET::GetCounter() const {
        volatile auto hpetRegs = static_cast<HPETRegisters*>(_hpetMappedAddress);
        return hpetRegs->MainCounterValue;
    }

    uint64_t HPET::GetNanoseconds() const {
        uint64_t total;
        uint32_t last;

        // Save current RFLAGS and disable interrupts
        uint64_t rflags;
        asm volatile(
            "pushfq\n\t"
            "popq %0\n\t"
            "cli"
            : "=r"(rflags)
            :
            : "memory"
        );

        total = _totalTicks;
        last = _lastCounter;

        // Restore previous RFLAGS (restores previous IF bit state)
        asm volatile(
            "pushq %0\n\t"
            "popfq"
            :
            : "r"(rflags)
            : "memory", "cc"
        );

        auto current = static_cast<uint32_t>(GetCounter());
        uint32_t delta = current - last;

        return (total + delta) * 1'000'000'000ULL / _hpetFrequency;
    }

    uint64_t HPET::GetFrequency() const {
        return _hpetFrequency;
    }

    void HPET::BusyWait(uint64_t nanoseconds) const {
        uint64_t startTime = GetNanoseconds();
        while (GetNanoseconds() - startTime < nanoseconds) {
        }
    }

    void HPET::Tick() {
        auto current32 = static_cast<uint32_t>(GetCounter());
        auto last32 = static_cast<uint32_t>(_lastCounter);
        auto delta = current32 - last32; // Standard 32-bit wrap-around math
        _totalTicks += delta;
        _lastCounter = current32;
    }
}
