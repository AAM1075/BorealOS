#include "FramebufferConsole.h"

#include "Boot/LimineDefinitions.h"
#include "Threading/Spinlock.h"
#include "Utility/ANSI.h"
#include "Utility/cstring.h"

namespace IO {
    void FramebufferConsole::Initialize() {
        if (!Boot::Limine::FramebufferRequest.response || Boot::Limine::FramebufferRequest.response->framebuffer_count <= 0)
            PANIC("Framebuffer not available");

        _framebuffer = Boot::Limine::FramebufferRequest.response->framebuffers[0];

        _ftContext = flanterm_fb_init(
            nullptr,
            nullptr,
            reinterpret_cast<uint32_t*>(_framebuffer->address),
            _framebuffer->width,
            _framebuffer->height,
            _framebuffer->pitch,
            _framebuffer->red_mask_size,
            _framebuffer->red_mask_shift,
            _framebuffer->green_mask_size,
            _framebuffer->green_mask_shift,
            _framebuffer->blue_mask_size,
            _framebuffer->blue_mask_shift,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            8,
            16,
            1,
            1,
            1,
            0,
            FLANTERM_FB_ROTATE_0
        );

        if (!_ftContext)
            PANIC("Failed to initialize Flanterm framebuffer context");

        _initialized = true;

        Write(Utility::ANSI::Colors::Background::OURBLE);
        Write(Utility::ANSI::Colors::Foreground::White);
        Write(Utility::ANSI::EscapeCodes::ClearScreen);
    }

    void FramebufferConsole::Write(const char *buff, size_t size) {
        Threading::ScopedLock lock(_lock, true);
        if (_initialized)
            flanterm_write(_ftContext, buff, size);
    }

    void FramebufferConsole::Write(const char *buff) {
        if (_initialized)
            Write(buff, strlen(buff));
    }
}
