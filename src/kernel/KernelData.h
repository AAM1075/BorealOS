#ifndef BOREALOS_KERNELDATA_H
#define BOREALOS_KERNELDATA_H

#include <Logging.h>

#include "IO/FramebufferConsole.h"
#include "Utility/CommandLineExtractor.h"

struct KernelData {
    IO::FramebufferConsole framebufferConsole;
    Utility::CommandLineExtractor commandLineExtractor;
    LOG_LEVEL minimumLogLevel;
};

#endif //BOREALOS_KERNELDATA_H
