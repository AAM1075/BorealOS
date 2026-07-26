#ifndef BOREALOS_GDT_H
#define BOREALOS_GDT_H

#include <Definitions.h>

namespace Interrupts {
    class GDT {
    private:
        struct PACKED GDTEntry {
            uint16_t LimitLow;
            uint16_t BaseLow;
            uint8_t BaseMiddle;
            uint8_t Access;
            uint8_t Granularity;
            uint8_t BaseHigh;
        };

        struct PACKED GDTPointer {
            uint16_t Limit;
            uint64_t Base;
        };

        static uint64_t entries[Architecture::MaxCPUs][8]; // Null, Code, Data, TSS, UserCode, UserData
        static GDTPointer gdtp[Architecture::MaxCPUs];
        static void SetEntry(uint32_t cpuId, int index, uint32_t base, uint32_t limit, uint8_t access, uint8_t granularity);
        static void SetTSSDescriptor(uint32_t cpuId, int index, uint64_t base, uint32_t limit);

    public:
        static void Initialize(uint32_t cpuId, void *rsp0, void *ts1);
        static GDTPointer* GetGDTPointer(uint32_t cpuId);
        static uint32_t GetCpuId();
    };
} // Interrupts

#endif //BOREALOS_GDT_H
