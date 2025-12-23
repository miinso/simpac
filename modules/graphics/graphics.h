#pragma once

#include <flecs.h>
#include <raylib.h>
#include <raymath.h>

#include <functional>

#ifdef __EMSCRIPTEN__
#include <emscripten/threading.h>
#endif

// Sub-headers
#include "camera.h"
#include "lighting.h"
#include "pipelines.h"
#include "resources.h"

// Platform-specific GLSL version
#if defined(PLATFORM_DESKTOP)
    inline constexpr int GLSL_VERSION = 330;
#else  // PLATFORM_WEB, PLATFORM_ANDROID
    inline constexpr int GLSL_VERSION = 100;
#endif

namespace graphics {

// Flecs phase entities - assigned during init()
// Use with .kind(graphics::PreRender) etc. when registering systems
inline flecs::entity phase_pre_render;
inline flecs::entity phase_on_render;
inline flecs::entity phase_post_render;
inline flecs::entity phase_on_present;

// Internal state
namespace detail {
    inline flecs::world* ecs = nullptr;
    inline std::function<void()> update_func;
    inline bool draw_grid = true;
    inline bool draw_fps = true;
    inline int grid_slices = 12;
    inline float grid_spacing = 10.0f / 12.0f;
    inline Font font = {0};
    inline bool font_loaded = false;
}

// Get the loaded font (falls back to default if not loaded)
[[nodiscard]] inline Font get_font() {
    return detail::font_loaded ? detail::font : GetFontDefault();
}

// Configuration for window and rendering
struct WindowConfig {
    int width = 800;
    int height = 600;
    const char* title = "Graphics";
    int target_fps = 60;
    Color clear_color = RAYWHITE;
};

// Check if we're on the render thread (for WASM threading safety)
[[nodiscard]] inline bool is_render_thread() {
#ifdef __EMSCRIPTEN__
    return emscripten_is_main_browser_thread();
#else
    return true;
#endif
}

// Initialize graphics module with flecs world
// Sets up rendering phases and core systems
void init(flecs::world& world);

// Initialize window with config
void init_window(const WindowConfig& config);

// Convenience overload
inline void init_window(int width, int height, const char* title) {
    init_window({width, height, title});
}

// Run the main loop
// Optional update function called each frame after ECS progress
void run_main_loop(std::function<void()> update = nullptr);

// Check if window should close
[[nodiscard]] bool window_should_close();

// Close window and cleanup
void close_window();

// Toggle grid rendering
inline void set_draw_grid(bool draw, int slices = 12, float spacing = 10.0f / 12.0f) {
    detail::draw_grid = draw;
    detail::grid_slices = slices;
    detail::grid_spacing = spacing;
}

// Toggle FPS counter
inline void set_draw_fps(bool draw) {
    detail::draw_fps = draw;
}

} // namespace graphics
