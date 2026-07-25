#ifndef BOREALOS_PARAMETERS_H
#define BOREALOS_PARAMETERS_H

#include "Utility/StringView.h"

// This file defines all parameters that can be passed into the command line, excluding those defined by other modules.

namespace Parameters {
    constexpr Utility::StringView LOG_LEVEL = "LOG_LEVEL";
}

#endif //BOREALOS_PARAMETERS_H
