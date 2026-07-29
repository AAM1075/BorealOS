#include <cpuid.h>
#include "CPU.h"
#include "Logging.h"
#include "Boot/LimineDefinitions.h"
#include "Utility/StringUtils.h"

void Core::CPU::Initialize() {
    // Get the CPU brand string
    ReadBrandString(_cpuName);
    _cpuNameLength = strlen(_cpuName);
    LOG_INFO("System processor \"{}\" has {} core(s)", GetCPUName(), GetCoreCount());
}

void Core::CPU::InitializeCore(uint16_t CPUID) {
    // Initialize SSE, the FPU, and AVX so vector math and floating-point numbers work without exploding the CPU
    InitializeSIMD(CPUID);

    // Enable No-eXecute (NX) to stop code execution from data-only memory, SECURITY(tm) WOOOO
    InitializeNX(CPUID);
}

void Core::CPU::InitializeSIMD(uint16_t CPUID) {
    uint32_t mxcsr = 0x1F80;
    uint16_t controlWord = 0x00, statusWord = 0x5A5A;
    size_t cr0, cr4;

    // Query CPUID to check the supported SIMD extensions
    uint32_t cpuidEAX = 0, cpuidEBX = 0, cpuidECX = 0, cpuidEDX = 0;
    asm volatile(
        "cpuid"
        : "=a"(cpuidEAX), "=b"(cpuidEBX), "=c"(cpuidECX), "=d"(cpuidEDX)
        : "a"(1)
    );

    // The FPU and SSE are required, but AVX is optional
    bool hasFPU = (cpuidEDX & (1 << 0)) != 0;
    bool hasSSE = (cpuidEDX & (1 << 25)) != 0;
    bool hasAVX = (cpuidECX & (1 << 28)) != 0;

    if (!hasFPU || !hasSSE) {
        LOG_ERROR("Core #{} does not support one or more critical SIMD extensions (FPU: {}, SSE: {})!", CPUID, hasFPU, hasSSE);
        PANIC("One or more CPU cores does not support critical SIMD extensions!");
    }

    if (!hasAVX) {
        LOG_WARNING("Core #{} does not support AVX", CPUID);
    }

    // Reset and verify the FPU hardware
    asm volatile("fninit");
    asm volatile("fnstsw %0" : "=m"(statusWord));
    if (statusWord != 0) {
        LOG_ERROR("Core #{}: FPU status word mismatch ({} != 0x0)!", CPUID, (void*)(uintptr_t)statusWord);
        PANIC("SIMD initialization failed: FPU status word mismatch!");
    }

    asm volatile("fnstcw %0" : "=m"(controlWord));
    if ((controlWord & 0x103F) != 0x003F) {
        LOG_ERROR("Core #{}: FPU control word mismatch ({} != 0x003F)!", CPUID, (void*)(uintptr_t)controlWord);
        PANIC("SIMD initialization failed: FPU control word mismatch!");
    }

    // Configure EM/MP/TS in the CR0 register (disable emulation, set MP, clear TS)
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 &= ~(1 << 2);
    cr0 |= (1 << 1);
    cr0 &= ~(1 << 3);
    asm volatile("mov %0, %%cr0" :: "r"(cr0));

    // Configure CR4 for SSE (OSFXSR) and AVX (OSXSAVE, if supported)
    asm volatile("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= (1 << 9); // OSFXSR for SSE
    if (hasAVX) { cr4 |= (1 << 18); } // OSXSAVE for AVX
    asm volatile("mov %0, %%cr4" :: "r"(cr4));

    // Enable X87, SSE, and AVX states in the XCR0 register
    if (hasAVX) {
        uint32_t eax, edx;
        asm volatile("xgetbv" : "=a" (eax), "=d" (edx) : "c" (0));
        eax |= 7; // Bits 0 (x87), 1 (SSE), and 2 (AVX)
        asm volatile("xsetbv" : : "a" (eax), "d" (edx), "c" (0));
    }

    // Finalize the FPU control word, 0x37F is the default for masked exceptions and standard rounding
    asm volatile("fninit");
    controlWord = 0x37F;
    asm volatile("fldcw %0" :: "m"(controlWord));
    asm volatile("ldmxcsr %0" :: "m"(mxcsr));

    // Test the FPU to make sure nothing is fucked up
    float fpuTest = 1.5f + 2.5f;
    if (fpuTest != 4.0f) {
        LOG_ERROR("Core #{}: FPU test failed ({} != 4.0)!", CPUID, fpuTest);
        PANIC("SIMD initialization failed: FPU test failed!");
    }

    // Test SSE to make sure nothing is fucked up
    float a[2] = { 1.0f, 4.2f };
    float b[2] = { -5.0f, 6.9f };
    float result[2] = { 0 };

    asm volatile(
        "movups (%1), %%xmm0\n\t"
        "movups (%2), %%xmm1\n\t"
        "addps %%xmm1, %%xmm0\n\t"
        "movups %%xmm0, (%0)\n\t"
        :
        : "r"(result), "r"(a), "r"(b)
        : "xmm0", "xmm1", "memory"
    );

    if (result[0] != -4.0f || result[1] != 11.1f) {
        LOG_ERROR("Core #{}: SSE test failed ({} != -4.0 or {} != 11.1)!", CPUID, result[0], result[1]);
        PANIC("SIMD initialization failed: SSE test failed!");
    }

    // Test AVX with 256-bit YMM registers instead of 128-bit XMM registers, all elements should equal 9.0f
    if (hasAVX) {
        float avxA[8] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f };
        float avxB[8] = { 8.0f, 7.0f, 6.0f, 5.0f, 4.0f, 3.0f, 2.0f, 1.0f };
        float avxResult[8] = { 0 };

        asm volatile(
            "vmovups (%1), %%ymm0\n\t"
            "vmovups (%2), %%ymm1\n\t"
            "vaddps %%ymm1, %%ymm0, %%ymm0\n\t"
            "vmovups %%ymm0, (%0)\n\t"
            :
            : "r"(avxResult), "r"(avxA), "r"(avxB)
            : "ymm0", "ymm1", "memory"
        );

        bool avxPassed = true;
        for (float i : avxResult) {
            if (i != 9.0f) {
                avxPassed = false;
                break;
            }
        }

        if (!avxPassed) {
            LOG_ERROR("Core #{}: AVX test failed, it will not be usable!", CPUID);
            hasAVX = false;
        }
    }

    LOG_INFO("Core #{}: SIMD extensions initialized (FPU, SSE, {})", CPUID, hasAVX ? "AVX" : "no AVX");
}

void Core::CPU::InitializeNX(uint16_t CPUID) {
    unsigned int eax, ebx, ecx, edx;

    // This core must have MSRs for NX to work
    if (!CoreHasMSR()) {
        LOG_ERROR("Core #{} does not support Model Specific registers!", CPUID);
        PANIC("No-eXecute initialization failed, one or more cores do not support Model Specific Registers!");
    }

    // Check if this core supports extended functions
    if (!__get_cpuid(LEAF_EXTENDED_FUNC_PARAMETER, &eax, &ebx, &ecx, &edx) || eax < LEAF_EXTENDED_PROCESSOR_INFO_FEATURES) {
        LOG_ERROR("Core #{} does not support extended CPUID leaves!", CPUID);
        PANIC("No-eXecute initialization failed, extended leaves unsupported!");
    }

    // Check if bit 20 of leaf 0x80000001 to see if this core supports NX
    if (__get_cpuid(LEAF_EXTENDED_PROCESSOR_INFO_FEATURES, &eax, &ebx, &ecx, &edx)) {
        if ((edx & (1 << 20)) == 0) {
            LOG_ERROR("Core #{} does not support No-eXecute!", CPUID);
            PANIC("No-eXecute initialization failed, one or more cores do not support No-eXecute!");
        }
    }
    else {
        LOG_ERROR("Core #{} failed to query CPUID leaf 0x80000001!", CPUID);
        PANIC("No-eXecute initialization failed!");
    }

    // The Extended Feature Enable Register (EFER) is bit 11 in the MSR
    uint64_t MSRResult = ReadMSR(MSR_EXTENDED_FEATURE_ENABLE);
    MSRResult |= (1ULL << 11);
    WriteMSR(MSR_EXTENDED_FEATURE_ENABLE, MSRResult);
    LOG_INFO("Initialized NX on core #{}", CPUID);
}

void Core::CPU::ReadBrandString(char* buffer) {
    auto* currentChunk = reinterpret_cast<uint32_t*>(buffer);

    // The brand string is contained inside CPUID leaves 0x80000002 to 0x80000004
    // NOTE: On x86_64, the CPUID instruction clobbers the RBX register, so we need to push and pop it for preservation
    for (uint32_t leaf = 0x80000002; leaf <= 0x80000004; ++leaf) {
        asm volatile(
            "pushq %%rbx\n\t"
            "cpuid\n\t"
            "movl %%ebx, %1\n\t"
            "popq %%rbx"
            : "=a"(currentChunk[0]), "=r"(currentChunk[1]), "=c"(currentChunk[2]), "=d"(currentChunk[3]) // Using '=r' lets this assembly manually move data out of EBX and into an arbitrary register
            : "a"(leaf)
            : "cc" // We need to tell the compiler that the CPU status flags have been altered
        );

        currentChunk += 4;
    }

    // Eliminate trailing whitespace from the brand string and null terminate it
    // NOTE: This implementation forces a null termination at the end of the brand string buffer for safety
    buffer[48] = '\0';
    for (int charIndex = 47; charIndex >= 0; charIndex--) {
        if (buffer[charIndex] == '\0' || Utility::StrUtils::IsCharWhitespace(buffer[charIndex])) {
            buffer[charIndex] = '\0';
        }
        else { break; }
    }

    // Eliminate leading whitespace by shifting the buffer contents to the front of the buffer
    uint8_t startIndex = 0;
    while (buffer[startIndex] != '\0' && Utility::StrUtils::IsCharWhitespace(buffer[startIndex])) { startIndex++; } // Find the first non-whitespace character
    if (startIndex > 0) {
        uint8_t shiftIndex = 0;
        while (buffer[startIndex + 1] != '\0') {
            buffer[shiftIndex] = buffer[startIndex + 1];
            shiftIndex++;
        }

        // Place a new null terminator at the end of the brand string since the contents have moved
        buffer[shiftIndex] = '\0';
    }
}

void Core::CPU::WriteMSR(uint32_t MSR, uint64_t data) {
    // Split the data into in half
    auto high = (uint32_t)(data >> 32);
    auto low = (uint32_t)data;

    // Write the low and high components into the MSR
    asm volatile(
        "wrmsr"
        :: "a"(low), "d"(high), "c"(MSR)
    );
}

bool Core::CPU::CoreHasMSR() {
    unsigned int eax, ebx, ecx, edx;

    // Call CPUID with the processor info & features leaf
    // NOTE: Bit 5 in EDX is the MSR feature flag, the CPU has MSRs if this bit is set
    if (__get_cpuid(LEAF_PROCESSOR_INFO_FEATURES, &eax, &ebx, &ecx, &edx)) {
        return (edx & (1 << 5)) != 0;
    }

    return false;
}

uint64_t Core::CPU::GetCoreCount() {
    return Boot::Limine::MultiProcessingRequest.response->cpu_count;
}

uint64_t Core::CPU::ReadMSR(uint32_t MSR) {
    uint32_t high, low;

    asm volatile(
        "rdmsr"
        : "=a"(low), "=d"(high)
        : "c"(MSR)
    );

    // Combine the low and high DWORDS into one QWORD
    return ((uint64_t)high << 32) | low;
}

Utility::StringView Core::CPU::GetCPUName() {
    return {_cpuName, _cpuNameLength};
}