#ifndef BOREALOS_TSS_H
#define BOREALOS_TSS_H

#include <Definitions.h>

namespace Interrupts {
    class TSS {
    public:
        struct PACKED TSSStruct {
            uint32_t Reserved0;
            uint64_t RSP0;
            uint64_t RSP1;
            uint64_t RSP2;
            uint64_t Reserved1;
            uint64_t IST1;
            uint64_t IST2;
            uint64_t IST3;
            uint64_t IST4;
            uint64_t IST5;
            uint64_t IST6;
            uint64_t IST7;
            uint64_t Reserved2;
            uint16_t Reserved3;
            uint16_t IOMapBase;
        };

        static void Initialize(uint32_t cpuId, void *rsp0, void *ist1);
        static TSSStruct* GetTSSStruct(uint32_t cpuId);
    private:
        static TSSStruct _tss[Architecture::MaxCPUs];
    };
} // Interrupts

#endif //BOREALOS_TSS_H