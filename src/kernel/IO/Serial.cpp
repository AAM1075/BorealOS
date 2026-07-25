// RS232 Serial Driver
#include "PortIO.h"
#include "Serial.h"

static bool portsInitStatus[8] = {false, false, false, false, false, false, false, false};

static int GetPortIndex(uint16_t comPort) {
    switch (comPort) {
        case IO::Serial::COM1: return 0;
        case IO::Serial::COM2: return 1;
        case IO::Serial::COM3: return 2;
        case IO::Serial::COM4: return 3;
        case IO::Serial::COM5: return 4;
        case IO::Serial::COM6: return 5;
        case IO::Serial::COM7: return 6;
        case IO::Serial::COM8: return 7;
        default: return -1;
    }
}

// === READ & WRITE ===
void IO::Serial::WriteString(uint16_t comPort, const char *str) {
    while (*str) {
        WriteChar(comPort, *str++);
    }
}

void IO::Serial::WriteChar(uint16_t comPort, char c) {
    while (TransmitEmpty(comPort) == 0) {}
    outb(comPort, c);
}

// === PORT STATUS ===
// Initialize am RS232 COM port
// NOTE: See IO::Serial for port definitions
void IO::Serial::Initialize(uint16_t comPort) {
    // We can skip the loopback test because we have "modern" serial controller ICs; we rarely have to worry about miswired or faulty UART systems
    // NOTE: See https://wiki.osdev.org/Serial_Ports#Initialization
    outb(comPort + 1, 0x00); // Disable all interrupts

    // Set the divisor to 0x0003 for 38400 baud. Write the divisor value in little endian format (low byte first!)
    outb(comPort + 3, 0x80);
    outb(comPort + 0, 0x03);
    outb(comPort + 1, 0x00);

    outb(comPort + 3, 0x03); // Configure transmission settings for 8 bits, no parity, and one stop bit
    outb(comPort + 2, 0xC7); // Enable FIFO and clear with a 14 byte threshold
    outb(comPort + 4, 0x0B); // Enable IRQs and set Request To Send / Data Set Ready

    // The serial port can only be marked as initialized if it exists
    if (!PortExists(comPort)) {
        PANIC("Serial port initialization failed!");
    }

    portsInitStatus[GetPortIndex(comPort)] = true;
    PRINT("Serial port initialized.");
}

// Check if a port exists and is functioning by checking if the transmit buffer is empty
bool IO::Serial::PortExists(uint16_t comPort) {
    constexpr int maxRetries = 1000;
    for (int i = 0; i < maxRetries; i++) {
        if (TransmitEmpty(comPort)) return true;
        IOWait();
    }

    return false;
}

bool IO::Serial::PortInitialized(uint16_t comPort) {
    return portsInitStatus[GetPortIndex(comPort)];
}

uint32_t IO::Serial::TransmitEmpty(uint16_t comPort) {
    return inb(comPort + 5) & 0x20;
}

uint32_t IO::Serial::Available(uint16_t comPort) {
    return inb(comPort + 5) & 0x01;
}