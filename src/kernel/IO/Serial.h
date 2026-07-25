// Header for the x64 RS232 Serial driver
#ifndef BOREALOS_SERIAL_H
#define BOREALOS_SERIAL_H

#include <Definitions.h>

namespace IO::Serial {
    // Default I/O ports for the 8 COM devices
    constexpr uint16_t COM1 = 0x3F8;
    constexpr uint16_t COM2 = 0x2F8;
    constexpr uint16_t COM3 = 0x3E8;
    constexpr uint16_t COM4 = 0x2E8;
    constexpr uint16_t COM5 = 0x5F8;
    constexpr uint16_t COM6 = 0x4F8;
    constexpr uint16_t COM7 = 0x5E8;
    constexpr uint16_t COM8 = 0x4E8;

    // In & out
    void WriteString(uint16_t comPort, const char* str);
    void WriteChar(uint16_t comPort, char c);

    // Port status
    uint32_t TransmitEmpty(uint16_t comPort);
    uint32_t Available(uint16_t comPort);
    bool PortInitialized(uint16_t comPort);
    bool PortExists(uint16_t comPort);
    void Initialize(uint16_t comPort);
}

#endif //BOREALOS_SERIAL_H
