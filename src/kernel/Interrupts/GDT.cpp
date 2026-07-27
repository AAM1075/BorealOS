#include "GDT.h"
#include "TSS.h"

constexpr uintptr_t _gsOffset = 0xC0000101; // TODO: use the CPU class for this.

extern "C" {
    extern void LoadGDT(uint64_t gdt_pointer);
    extern void LoadTSS(uint16_t tss_selector);
}

namespace Interrupts {
    uint64_t GDT::entries[Architecture::MaxCPUs][8]; // Null, Code, Data, TSS low and high, UserCode, UserData
    GDT::GDTPointer GDT::gdtp[Architecture::MaxCPUs];

    void GDT::SetEntry(uint32_t cpuId, int index, uint32_t base, uint32_t limit, uint8_t access, uint8_t granularity) {
        GDTEntry entry = {};
        entry.LimitLow = limit & 0xFFFF;
        entry.BaseLow = base & 0xFFFF;
        entry.BaseMiddle = (base >> 16) & 0xFF;
        entry.Access = access;
        entry.Granularity = (limit >> 16) & 0x0F;
        entry.Granularity |= granularity & 0xF0;
        entry.BaseHigh = (base >> 24) & 0xFF;

        __builtin_memcpy(&entries[cpuId][index], &entry, sizeof(GDTEntry));
    }

    void GDT::SetTSSDescriptor(uint32_t cpuId, int index, uint64_t base, uint32_t limit) {
        uint64_t low = 0;
        uint64_t high = 0;

        low |= (limit & 0xFFFFULL);
        low |= (base & 0xFFFFFFULL) << 16;
        low |= (0x89ULL) << 40;
        low |= ((limit >> 16) & 0xFULL) << 48;
        low |= ((base >> 24) & 0xFFULL) << 56;

        high |= (base >> 32) & 0xFFFFFFFFULL;

        entries[cpuId][index] = low;
        entries[cpuId][index + 1] = high;
    }

    uint32_t GDT::GetCpuId() {
        // Read out GS.base
        uint64_t output;
        asm volatile ("rdmsr" : "=a"(output) : "c"(_gsOffset));
        return static_cast<uint32_t>(output);
    }

    void GDT::Initialize(uint32_t cpuId, void *rsp0, void *ts1) {
        TSS::Initialize(cpuId, rsp0, ts1);
        auto tss_address = reinterpret_cast<uintptr_t>(TSS::GetTSSStruct(cpuId));

        // Null descriptor
        SetEntry(cpuId, 0, 0, 0, 0, 0);

        // Code segment descriptor
        SetEntry(cpuId, 1, 0, 0xFFFFF, 0x9A, 0xA0);

        // Data segment descriptor
        SetEntry(cpuId, 2, 0, 0xFFFFF, 0x92, 0xA0);

        // TSS descriptor
        SetTSSDescriptor(cpuId, 3, tss_address, sizeof(TSS::TSSStruct) - 1);

        // User mode code segment descriptor
        SetEntry(cpuId, 5, 0, 0xFFFFF, 0xF2, 0xC0); // star anchor
        SetEntry(cpuId, 6, 0, 0xFFFFF, 0xF2, 0xA0); // UserData 64 DPL=3 SS on SYSRET
        SetEntry(cpuId, 7, 0, 0xFFFFF, 0xFA, 0xA0); // UserCode 64 DPL=3 CS on SYSRET, L bit set

        gdtp[cpuId].Limit = sizeof(entries[cpuId]) - 1;
        gdtp[cpuId].Base = reinterpret_cast<uint64_t>(&entries[cpuId]);

        LoadGDT(reinterpret_cast<uint64_t>(&gdtp[cpuId]));
        LoadTSS(0x18); // TSS selector is at offset 0x18 in the GDT (3rd entry, 3 * 8 = 0x18)

        asm volatile ("wrmsr" : : "c"(_gsOffset), "a"(cpuId & 0xFFFFFFFF), "d"((uint64_t)cpuId >> 32));
    }

    GDT::GDTPointer * GDT::GetGDTPointer(uint32_t cpuId) {
        return &gdtp[cpuId];
    }
} // Interrupts