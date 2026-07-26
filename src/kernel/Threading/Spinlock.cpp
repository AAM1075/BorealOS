#include "Spinlock.h"

Threading::Spinlock::Spinlock()  {
    __atomic_store_n(&_locked, false, __ATOMIC_RELEASE);
}

// TODO: Use the CPU class for this
bool AreInterruptsEnabled() {
    uint64_t rflags;

    asm volatile (
        "pushfq\n\t"
        "pop %0"
        : "=rm" (rflags)
        :
        : "memory"
    );

    return (rflags & (1 << 9)) != 0;
}

bool Threading::Spinlock::Lock(bool stopInterrupts) {
    bool hadInterruptsEnabled = AreInterruptsEnabled();
    if (stopInterrupts) {
        asm volatile ("cli" ::: "memory");
    }

    bool expected;
    do {
        while (__atomic_load_n(&_locked, __ATOMIC_RELAXED))
            asm volatile ("pause");
        expected = false;
    } while (!__atomic_compare_exchange_n(&_locked, &expected, true, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED));

    return (stopInterrupts && hadInterruptsEnabled);
}

void Threading::Spinlock::Unlock(bool reenableInterrupts) {
    __atomic_store_n(&_locked, false, __ATOMIC_RELEASE);

    if (reenableInterrupts) {
        asm volatile ("sti" ::: "memory");
    }
}

Threading::ScopedLock::ScopedLock(Spinlock &lock, bool stopInterrupts) : _lock(lock) {
    _reenableInterrupts = _lock.Lock(stopInterrupts);
}

Threading::ScopedLock::~ScopedLock() {
    _lock.Unlock(_reenableInterrupts);
}