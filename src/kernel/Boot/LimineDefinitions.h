#ifndef BOREALOS_LIMINEDEFINITIONS_H
#define BOREALOS_LIMINEDEFINITIONS_H

#include <Definitions.h>
#include "limine.h"

namespace Boot::Limine {
    extern volatile uint64_t LimineBaseRevision[];
    extern volatile struct limine_framebuffer_request FramebufferRequest;
    extern volatile struct limine_memmap_request MemmapRequest;
    extern volatile struct limine_hhdm_request HhdmRequest;
    extern volatile struct limine_module_request ModuleRequest;
    extern volatile struct limine_rsdp_request RsdpRequest;
    extern volatile struct limine_executable_cmdline_request CommandlineRequest;
    extern volatile struct limine_mp_request MultiProcessingRequest;
    extern volatile uint64_t LimineRequestsStartMarker[];
    extern volatile uint64_t LimineRequestsEndMarker[];
}

#endif //BOREALOS_LIMINEDEFINITIONS_H
