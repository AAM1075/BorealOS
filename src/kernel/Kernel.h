#ifndef BOREALOS_KERNEL_H
#define BOREALOS_KERNEL_H

#include <Definitions.h>
#include "KernelData.h"

class Kernel {
public:
    void Initialize();
    void Start();
    void Log(const char* message, size_t c);
    [[noreturn]] void Panic(const char* message);
    static Kernel& GetInstance();
    CpuData* GetCpuData();

    KernelData Data;
    CpuData Cpu[Architecture::MaxCPUs];
};

#endif //BOREALOS_KERNEL_H
