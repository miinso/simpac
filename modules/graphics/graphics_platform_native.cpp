#include "graphics.h"

#include <cstdio>

namespace graphics::detail {
void platform_before_init_window(const WindowConfig&, CanvasSize&) {}

void platform_after_init_window(const WindowConfig&, const CanvasSize&) {}

void platform_set_target_fps(const WindowConfig& config) {
    SetTargetFPS(config.target_fps);
}

void platform_pre_close_window() {}

void platform_run_loop() {
    std::printf("graphics::invoking native while loop\n");
    while (!window_should_close()) {
        run_frame(GetFrameTime());
    }
    std::printf("graphics::destroying native while loop\n");
    close_window();
}
} // namespace graphics::detail
