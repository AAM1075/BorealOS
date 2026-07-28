#include "CPU.h"
#include "Logging.h"
#include "Boot/LimineDefinitions.h"
#include "Utility/StrUtils.h"

void CPU::InitializeCores(const limine_mp_response &mpResponse) {
    // Get the CPU brand string
    GetCPUName(cpuName);
    Utility::StringView cpuNameStrView = cpuName;
    const uint64_t coreCount = GetCoreCount();

    LOG_INFO("System processor \"{}\" has {} core(s)", cpuNameStrView, coreCount);

    // Initialize SSE and the FPU
    for (uint64_t coreIndex = 0; coreIndex < coreCount; coreIndex++) {
        const uint64_t CPUID = mpResponse.cpus[coreIndex]->processor_id;
        FPU::InitializeFPU(CPUID);
        SSE::InitializeSSE(CPUID);
    }

    LOG_INFO("Initialized SSE and FPUs on {} core(s)", coreCount);
}

uint64_t CPU::GetCoreCount() {
    return Boot::Limine::MultiProcessingRequest.response->cpu_count;
}

void CPU::GetCPUName(char* buffer) {
    auto* currentChunk = reinterpret_cast<uint32_t*>(buffer);

    // The brand string is contained inside CPUID leaves 0x80000002 to 0x80000004
    // NOTE: On x86_64, the CPUID iinstruction clobbers the RBX register, so we need to push and pop it for preservation
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

void CPU::FPU::InitializeFPU(uint16_t CPUID) {
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

void CPU::SSE::InitializeSSE(uint16_t CPUID) {
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
