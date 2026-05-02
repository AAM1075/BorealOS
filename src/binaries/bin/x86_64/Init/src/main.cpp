// BorealOS Init process.

// syscalls:
// 60 is exit
// 24 is to yield the CPU to the scheduler without exiting.
// 0 is to read from a file descriptor
// 1 is to write a string to the console
// 9 is to mmap memory

#include <cstddef>

static void* mmap_anon(size_t length) {
    void* result;
    register long r10 asm("r10") = 0x22; // MAP_PRIVATE | MAP_ANONYMOUS
    asm volatile (
        "syscall"
        : "=a"(result)
        : "a"(9L),
          "D"(0UL),
          "S"(length),
          "d"(0x03L), // PROT_READ | PROT_WRITE
          "r"(r10)         // compiler sees the register variable, emits r10
        : "rcx", "r11", "memory"
    );
    return result;
}

[[noreturn]] int main() {
    // This NEVER returns, and always loops.

    // TODO: add some init system stuff here later.

    // Allocate some memory using mmap and write to it, to test that mmap works.
    const char *mmapMessage = "Hello from the memory allocated by mmap! This is a test of the mmap syscall.\n\r";
    size_t messageLength = 0;
    while (mmapMessage[messageLength] != '\0') {
        messageLength++;
    }

    char* mmapMemory = (char*)mmap_anon(messageLength + 1);
    for (size_t i = 0; i < messageLength; i++) {
        mmapMemory[i] = mmapMessage[i];
    }
    mmapMemory[messageLength] = '\0';

    while (true) {
        asm volatile (
            "syscall"
            :
            : "a"(1L), // syscall number for write
              "D"(1UL), // fd = 1 for stdout
              "S"(mmapMemory), // buffer
              "d"(messageLength) // count
            : "rcx", "r11", "memory"
        );

        // Yield to the scheduler to allow other processes to run.
        asm volatile (
            "syscall"
            :
            : "a"(24)
            : "rcx", "r11", "memory"
        );
    }
}