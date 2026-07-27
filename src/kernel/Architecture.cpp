#include <Definitions.h>

extern "C" {
    extern char _kernel_stack_top[];
    extern char _kernel_stack_bottom[];
    extern char _kernel_base[];
    extern char _kernel_end[];
    extern char _fault_handler_stack_bottom[];
    extern char _fault_handler_stack_top[];
}

namespace Architecture {
    volatile uintptr_t *StackTop = reinterpret_cast<uintptr_t *>(&_kernel_stack_top[0]);
    volatile uintptr_t *StackBottom = reinterpret_cast<uintptr_t *>(&_kernel_stack_bottom[0]);
    volatile size_t StackSize = (StackTop - StackBottom);
    volatile uintptr_t *KernelBase = reinterpret_cast<uintptr_t *>(&_kernel_base[0]);
    volatile uintptr_t *KernelEnd = reinterpret_cast<uintptr_t *>(&_kernel_end[0]);
    volatile size_t KernelSize = (KernelEnd - KernelBase);
    volatile uintptr_t *DefaultFaultHandlerTop = reinterpret_cast<uintptr_t *>(&_fault_handler_stack_top[0]);
    volatile uintptr_t *DefaultFaultHandlerBottom = reinterpret_cast<uintptr_t *>(&_fault_handler_stack_bottom[0]);
}