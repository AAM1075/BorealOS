#ifndef BOREALOS_CPU_H
#define BOREALOS_CPU_H

#include "Boot/limine.h"
#include "Utility/StringView.h"

namespace Core {
    class CPU {
    public:
        Utility::StringView GetCPUName();
        static uint64_t GetCoreCount();
        static uint64_t ReadMSR(uint32_t MSR);
        static void WriteMSR(uint32_t MSR, uint64_t data);
        static void InitializeCore(uint16_t CPUID);
        void Initialize();

    private:
        static bool CoreHasMSR();
        static void InitializeSIMD(uint16_t CPUID);
        static void InitializeNX(uint16_t CPUID);
        static void ReadBrandString(char* buffer);

        char _cpuName[49] = "\0";
        uint8_t _cpuNameLength = 0;

        // Model Specific Registers
        static constexpr uint32_t MSR_EXTENDED_FEATURE_ENABLE = 0xC0000080;

        // CPUID leaves
        static constexpr uint32_t LEAF_PROCESSOR_INFO_FEATURES = 0x00000001;
        static constexpr uint32_t LEAF_EXTENDED_FUNC_PARAMETER = 0x80000000;
        static constexpr uint32_t LEAF_EXTENDED_PROCESSOR_INFO_FEATURES = 0x80000001;
    };
}

#endif //BOREALOS_CPU_H
