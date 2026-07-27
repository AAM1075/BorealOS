// Header for the x64 RS232 Serial driver
#ifndef BOREALOS_SERIAL_H
#define BOREALOS_SERIAL_H

#include <Definitions.h>
#include "Threading/Spinlock.h"

namespace IO {
    class Serial{
    public:
        // Default I/O ports for the 8 COM devices
        static constexpr uint16_t COM1 = 0x3F8;
        static constexpr uint16_t COM2 = 0x2F8;
        static constexpr uint16_t COM3 = 0x3E8;
        static constexpr uint16_t COM4 = 0x2E8;
        static constexpr uint16_t COM5 = 0x5F8;
        static constexpr uint16_t COM6 = 0x4F8;
        static constexpr uint16_t COM7 = 0x5E8;
        static constexpr uint16_t COM8 = 0x4E8;

        explicit Serial(const uint16_t comPort)
            : _comPort(comPort) {
        }

        void Write(const char* data, size_t length);
        void Initialize();
        [[nodiscard]] uint32_t TransmitEmpty() const;
        [[nodiscard]] uint32_t Available() const;
        [[nodiscard]] bool PortInitialized() const;
        [[nodiscard]] bool PortExists() const;

    private:
        Threading::Spinlock _lock{};
        uint16_t _comPort;
    };
}

#endif //BOREALOS_SERIAL_H
