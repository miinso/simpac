#include "graphics.h"

#include <cstdio>

#include <emscripten/emscripten.h>

extern "C" __attribute__((weak)) void app_shutdown() {
    std::printf("Simulation has ended.\n");
}

namespace graphics::detail {
namespace {
    bool shutdown_requested = false;

    void main_loop_callback() {
        if (shutdown_requested) {
            return;
        }
        run_frame(0);
    }
} // namespace

void platform_before_init_window(const WindowConfig& config, CanvasSize& canvas_size) {
    canvas_size.width = EM_ASM_INT({ return Module.canvas.width; });
    canvas_size.height = EM_ASM_INT({ return Module.canvas.height; });
    std::printf("graphics::init_window requested=%dx%d canvas=%dx%d\n",
        config.width, config.height, canvas_size.width, canvas_size.height);
}

void platform_after_init_window(const WindowConfig& config, const CanvasSize& canvas_size) {
    if (canvas_size.width > 0 && canvas_size.height > 0 &&
        (canvas_size.width != config.width || canvas_size.height != config.height)) {
        SetWindowSize(canvas_size.width, canvas_size.height);
    }
}

void platform_set_target_fps(const WindowConfig&) {}

void platform_pre_close_window() {
    if (shutdown_requested) {
        return;
    }
    shutdown_requested = true;

    emscripten_cancel_main_loop();
    finalize_world();
    app_shutdown();
    std::printf("graphics::emscripten rAF loop canceled\n");
}

void platform_run_loop() {
    shutdown_requested = false;
    std::printf("graphics::invoking emscripten rAF loop\n");
    emscripten_set_main_loop(main_loop_callback, 0, 1);
}
} // namespace graphics::detail

// ============================================================================
// Cursor Control (EM_JS)
// ============================================================================
EM_JS(void, engine_set_cursor, (bool visible, bool locked), {
    postMessage({type : 'cursor', payload : {visible : visible, locked : locked}}); 
});

namespace graphics::shim {
    void show_cursor(bool show) { engine_set_cursor(show, false); }
}

extern "C" {
EMSCRIPTEN_KEEPALIVE void emscripten_shutdown() { graphics::close_window(); }

EMSCRIPTEN_KEEPALIVE void emscripten_resize(int width, int height) {
    std::printf("graphics::emscripten_resize %dx%d\n", width, height);
    SetWindowSize(width, height);
}
// ... mouse/key exports ...
}

extern "C" {
// ... existing defined above ...

// touch snapshot (simpler than events)
EMSCRIPTEN_KEEPALIVE void engine_set_touch_count(int count) {
    graphics::shim::touch_count = count;
    // reset actives? no, set_touch handles writing.
    // we should clear inactive ones?
    for (int i = count; i < 10; ++i)
        graphics::shim::touches[i].active = false;
}

EMSCRIPTEN_KEEPALIVE void engine_set_touch(int i, float x, float y) {
    if (i >= 0 && i < 10) {
        graphics::shim::touches[i].active = true;
        graphics::shim::touches[i].pos = {x, y};
        graphics::shim::touches[i].id = i; // simplified ID
    }
}
}

// ============================================================================
// input event handlers (called from Worker JS)
// ============================================================================

extern "C" {
EMSCRIPTEN_KEEPALIVE void engine_mouse_move(float x, float y, float dx, float dy) {
    graphics::shim::mouse_pos = {x, y};
    graphics::shim::mouse_delta.x += dx;
    graphics::shim::mouse_delta.y += dy;
    // std::printf("mouse move: %.0f, %.0f\n", x, y);
}

EMSCRIPTEN_KEEPALIVE void engine_mouse_down(int button, float x, float y) {
    if (button >= 0 && button < 8)
        graphics::shim::mouse_btn[button] = true;
    graphics::shim::mouse_pos = {x, y};
    // std::printf("mouse down: btn=%d @ %.0f, %.0f\n", button, x, y);
}

EMSCRIPTEN_KEEPALIVE void engine_mouse_up(int button, float x, float y) {
    if (button >= 0 && button < 8)
        graphics::shim::mouse_btn[button] = false;
    graphics::shim::mouse_pos = {x, y};
    // std::printf("mouse up: btn=%d @ %.0f, %.0f\n", button, x, y);
}

EMSCRIPTEN_KEEPALIVE void engine_mouse_wheel(float dx, float dy) {
    constexpr float kWheelScale = 1.0f / 120.0f; // normalize pixel deltas to wheel "steps"
    graphics::shim::mouse_wheel += -dy * kWheelScale; // Raylib GetMouseWheelMove returns float Y
}

EMSCRIPTEN_KEEPALIVE void engine_mouse_enter(int entered) {
    graphics::shim::mouse_in_canvas = (entered != 0);
    if (!graphics::shim::mouse_in_canvas) {
        for (int i = 0; i < 8; ++i)
            graphics::shim::mouse_btn[i] = false;
    }
}

EMSCRIPTEN_KEEPALIVE void engine_key_down(int key) {
    if (key >= 0 && key < 512)
        graphics::shim::keys[key] = true;
}

EMSCRIPTEN_KEEPALIVE void engine_key_up(int key) {
    if (key >= 0 && key < 512)
        graphics::shim::keys[key] = false;
}
}
