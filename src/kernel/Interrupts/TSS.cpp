#include "TSS.h"

namespace Interrupts {
    TSS::TSSStruct TSS::_tss[Architecture::MaxCPUs];

    void TSS::Initialize(uint32_t cpuId, void *rsp0, void *ist1) {
        _tss[cpuId] = {};
        _tss[cpuId].RSP0 = reinterpret_cast<uint64_t>(rsp0);
        _tss[cpuId].IST1 = reinterpret_cast<uint64_t>(ist1);
        _tss[cpuId].IOMapBase = sizeof(TSSStruct);
    }

    TSS::TSSStruct* TSS::GetTSSStruct(uint32_t cpuId) {
        return &_tss[cpuId];
    }
} // Interrupts