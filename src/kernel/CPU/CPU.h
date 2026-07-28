#ifndef BOREALOS_CPU_H
#define BOREALOS_CPU_H

#include <Definitions.h>
#include "Boot/limine.h"

namespace CPU {
    void InitializeCores(const limine_mp_response &mpResponse);
    void GetCPUName(char* buffer);
    uint64_t GetCoreCount();
    inline char cpuName[49] = "\0";
}

namespace CPU::SSE {
    struct alignas(16) FXSaveArea {
        uint16_t fcw;
        uint16_t fsw;
        uint8_t  ftw;
        uint8_t  reserved1;
        uint16_t fop;
        uint64_t fpuIP;
        uint64_t fpuDP;
        uint32_t mxcsr;
        uint32_t mxcsrMask;
        uint8_t  mmxRegisters[8][16];
        uint8_t  xmmRegisters[16][16]; // XMM0 - XMM15
        uint8_t  reserved2[48];
        uint8_t  available[32];
    };

    void InitializeSSE(uint16_t CPUID);
}

namespace CPU::FPU {
    void InitializeFPU(uint16_t CPUID);
}

#endif //BOREALOS_CPU_H
