#pragma once

#include <raylib.h>

namespace graphics::input {
    // when true, camera/input consumers should ignore left-mouse drag this frame.
    inline bool capture_mouse_left = false;
}

// cursor control on WASM (OffscreenCanvas can't access DOM cursor styles)
#if defined(__EMSCRIPTEN__)
namespace graphics::shim {
    void show_cursor(bool show);
}
#define ShowCursor() graphics::shim::show_cursor(true)
#define HideCursor() graphics::shim::show_cursor(false)
#define EnableCursor() graphics::shim::show_cursor(true)
#define DisableCursor() graphics::shim::show_cursor(false)
#endif
