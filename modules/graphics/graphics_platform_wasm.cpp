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
}

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

extern "C" {
    EMSCRIPTEN_KEEPALIVE void emscripten_shutdown() {
        graphics::close_window();
    }

    EMSCRIPTEN_KEEPALIVE void emscripten_resize(int width, int height) {
        std::printf("graphics::emscripten_resize %dx%d\n", width, height);
        SetWindowSize(width, height);
    }
}
