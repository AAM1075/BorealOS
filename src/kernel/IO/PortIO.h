// x86_64 port IO functions
#ifndef BOREALOS_PORTIO_H
#define BOREALOS_PORTIO_H

#include <Definitions.h>
namespace IO::Port {
    void IOWait();
    void outb(uint16_t port, uint8_t value);
    void outw(uint16_t port, uint16_t value);
    void outl(uint16_t port, uint32_t value);

    uint8_t inb(uint16_t port);
    uint16_t inw(uint16_t port);
    uint32_t inl(uint16_t port);
}

#endif //BOREALOS_PORTIO_H
