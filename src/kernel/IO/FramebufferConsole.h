#ifndef BOREALOS_FRAMEBUFFERCONSOLE_H
#define BOREALOS_FRAMEBUFFERCONSOLE_H

#include <Definitions.h>
#include <flanterm.h>
#include <flanterm_backends/fb.h>
#include <Boot/limine.h>

#include "Threading/Spinlock.h"

namespace IO {
    class FramebufferConsole {
    public:
        void Initialize();
        void Write(const char *buff, size_t size);
        void Write(const char *buff);

    private:
        Threading::Spinlock _lock{};
        limine_framebuffer* _framebuffer{};
        struct flanterm_context* _ftContext{};
        bool _initialized = false;
    };
}

#endif //BOREALOS_FRAMEBUFFERCONSOLE_H
