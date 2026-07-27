// RS232 Serial Driver
#include "PortIO.h"
#include "Serial.h"
#include "Logging.h"

static bool portsInitStatus[8] = {false, false, false, false, false, false, false, false};

namespace IO {
    static int GetPortIndex(const uint16_t comPort) {
        switch (comPort) {
            case Serial::COM1: return 0;
            case Serial::COM2: return 1;
            case Serial::COM3: return 2;
            case Serial::COM4: return 3;
            case Serial::COM5: return 4;
            case Serial::COM6: return 5;
            case Serial::COM7: return 6;
            case Serial::COM8: return 7;
            default: return -1;
        }
    }

    Serial::Serial(uint16_t comPort) {
        _comPort = comPort;
    }

    void Serial::Write(const char* data, const size_t length) {
        Threading::ScopedLock lock(_lock, true);
        for (size_t charIndex = 0; charIndex < length; charIndex++) {
            while (TransmitEmpty() == 0) {}
            Port::outb(_comPort, data[charIndex]);
        }
    }

    // Initialize am RS232 COM port
    // NOTE: See IO::Serial for port definitions
    void Serial::Initialize() const {
        // We can skip the loopback test because we have "modern" serial controller ICs; we rarely have to worry about miswired or faulty UART systems anymore
        // NOTE: See https://wiki.osdev.org/Serial_Ports#Initialization
        Port::outb(_comPort + 1, 0x00); // Disable all interrupts

        // Set the divisor to 0x0003 for 38400 baud. Write the divisor value in little endian format (low byte first!)
        Port::outb(_comPort + 3, 0x80);
        Port::outb(_comPort + 0, 0x03);
        Port::outb(_comPort + 1, 0x00);

        Port::outb(_comPort + 3, 0x03); // Configure transmission settings for 8 bits, no parity, and one stop bit
        Port::outb(_comPort + 2, 0xC7); // Enable FIFO and clear with a 14 byte threshold
        Port::outb(_comPort + 4, 0x0B); // Enable IRQs and set Request To Send / Data Set Ready

        // The serial port can only be marked as initialized if it exists
        if (!PortExists()) {
            PANIC("Serial port initialization failed!");
        }

        portsInitStatus[GetPortIndex(_comPort)] = true;
        LOG_INFO("Serial port {} initialized.", (void*)(uintptr_t)_comPort);
    }

    // Check if a port exists and is functioning by checking if the transmit buffer is empty
    bool Serial::PortExists() const {
        constexpr int maxRetries = 1000;
        for (int i = 0; i < maxRetries; i++) {
            if (TransmitEmpty()) return true;
            Port::IOWait();
        }

        return false;
    }

    bool Serial::PortInitialized() const {
        return portsInitStatus[GetPortIndex(_comPort)];
    }

    uint32_t Serial::TransmitEmpty() const {
        return Port::inb(_comPort + 5) & 0x20;
    }

    uint32_t Serial::Available() const {
        return Port::inb(_comPort + 5) & 0x01;
    }
}
