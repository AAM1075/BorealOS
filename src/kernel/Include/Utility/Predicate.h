#ifndef BOREALOS_PREDICATE_H
#define BOREALOS_PREDICATE_H
#include "Optional.h"

namespace Utility {
    // A predicate is a function that takes an item of type T and returns a bool!
    template<typename T>
    using Predicate = bool(*)(const T&);
}

#endif //BOREALOS_PREDICATE_H
