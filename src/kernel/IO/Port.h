#ifndef BOREALOS_PORT_H
#define BOREALOS_PORT_H

#include <Definitions.h>

namespace IO::Port {
    // Read & write
    void outb(uint16_t port, uint8_t value);
    void outw(uint16_t port, uint16_t value);
    void outl(uint16_t port, uint32_t value);

    uint8_t inb(uint16_t port);
    uint16_t inw(uint16_t port);
    uint32_t inl(uint16_t port);

    // Timing
    void IOWait();
}

#endif //BOREALOS_PORT_H
