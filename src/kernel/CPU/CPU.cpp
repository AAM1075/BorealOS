#include <cpuid.h>
#include "CPU.h"
#include "Logging.h"
#include "Boot/LimineDefinitions.h"
#include "Utility/StrUtils.h"

void Core::CPU::Initialize() {
    // Get the CPU brand string
    GetCPUName(_cpuName);
    LOG_INFO("System processor \"{}\" has {} core(s)", ReadBrandString(), GetCoreCount());
}

void Core::CPU::InitializeCore(uint16_t CPUID) {
    // Initialize SSE and the FPU so vector math and floating-point numbers work without exploding the CPU
    FPU::InitializeFPU(CPUID);
    SSE::InitializeSSE(CPUID);

    // Enable No-eXecute (NX) to stop code execution from data-only memory, SECURITY(tm) WOOOO
    InitializeNX(CPUID);
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
}

void Core::CPU::GetCPUName(char* buffer) {
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
}

void Core::CPU::FPU::InitializeFPU(uint16_t CPUID) {
    uint16_t controlWord = 0x00, statusWord = 0x5A5A;
    size_t cr0, cr4;

    // Reset the FPU and make sure it exists via the status word
    asm volatile("fninit");
    asm volatile("fnstsw %0" : "=m"(statusWord));
    if (statusWord != 0) {
        LOG_ERROR("Core #{}: FPU status word mismatch ({} != 0x0)!", CPUID, (void*)(uintptr_t)statusWord);
        PANIC("FPU initialization failed, status word mismatch!");
    }

    // Verify the control word defaults
    asm volatile("fnstcw %0" : "=m"(controlWord));
    if ((controlWord & 0x103F) != 0x003F) {
        LOG_ERROR("Core #{}: FPU control word mismatch ({} != 0x003F)!", CPUID, (void*)(uintptr_t)controlWord);
        PANIC("FPU initialization failed, control word mismatch!");
    }

    // Modify the EM/MP/TS bits in the CR0 register
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 &= ~(1 << 2); // Disable software FPU emulation
    cr0 |= (1 << 1); // Set MP (Monitor Coprocessor)
    cr0 &= ~(1 << 3); // Clear TS (Task Switched)
    asm volatile("mov %0, %%cr0" :: "r"(cr0));

    // Enable OSFXSR for SSE and FXSAVE / FXRSTOR instructions
    asm volatile("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= (1 << 9);
    asm volatile("mov %0, %%cr4" :: "r"(cr4));

    // Reset the FPU's hardware state and set the control word to the default 0f 0x37F
    // NOTE: 0x37F tells the FPU to enable double-extended precision, set the round mode to the nearest even number, amd mask all Fof its exceptions so the kernel doesn't explode
    asm volatile("fninit");
    controlWord = 0x37F;
    asm volatile("fldcw %0" :: "m"(controlWord));

    // Test the FPU to make sure everything is working properly
    float testResult = 1.5f + 2.5f;
    if (testResult != 4.0f) {
        LOG_ERROR("Core #{}: FPU test failed, unexpected result received ({} != 4.0)!", CPUID, testResult);
        PANIC("FPU initialization failed, unexpected result received!");
    }
}

void Core::CPU::SSE::InitializeSSE(uint16_t CPUID) {
    uint32_t mxcsr = 0x1F80;

    // Set the MXCSR register to its default value
    asm volatile("ldmxcsr %0" :: "m"(mxcsr));

    // Perform a vector math test with the XMM registers
    float a[2] = { 1.0f, 4.2f};
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
        LOG_ERROR("Core #{}: SSE test failed, unexpected result received ({} != -4.0 or {} != 11.1)!", CPUID, result[0], result[1]);
        PANIC("SSE initialization failed, unexpected result received!");
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

    // Call CPUID with leaf 1
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

Utility::StringView Core::CPU::ReadBrandString() {
    return {_cpuName};
}