#ifndef BOREALOS_SPINLOCK_H
#define BOREALOS_SPINLOCK_H

#include <Definitions.h>

namespace Threading {
    // A basic spinlock
    class Spinlock {
    public:
        Spinlock();

        bool Lock(bool stopInterrupts); // waits until _locked is available (false), and then sets it to true.
        void Unlock(bool reenableInterrupts); // sets _locked to false.

    private:
        bool _locked;
    };

    // A RAII spinlock similar to std::scoped_lock
    class ScopedLock {
    public:
        ScopedLock(Spinlock& lock, bool stopInterrupts);
        ~ScopedLock();

        ScopedLock(const ScopedLock&) = delete;
        ScopedLock& operator=(const ScopedLock&) = delete;
        ScopedLock(ScopedLock&&) = delete;
        ScopedLock& operator=(ScopedLock&&) = delete;
        ScopedLock() = delete;
        ScopedLock& operator=(Spinlock&) = delete;

    private:
        Spinlock& _lock;
        bool _reenableInterrupts;
    };
}

#endif //BOREALOS_SPINLOCK_H
