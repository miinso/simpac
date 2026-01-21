#pragma once

#include <flecs.h>
#include <raylib.h>
#include <raymath.h>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <emscripten/threading.h>
#endif


// platform-specific GLSL version
#if defined(PLATFORM_DESKTOP)
inline constexpr int GLSL_VERSION = 330;
#else // PLATFORM_WEB, PLATFORM_ANDROID
inline constexpr int GLSL_VERSION = 100;
#endif

#include "input.h"

// sub-headers (shim enabled) (order matters)
#include "camera.h"
#include "lighting.h"
#include "pipelines.h"
#include "resources.h"


namespace graphics {

// flecs phase entities - assigned during init().
// use with .kind(graphics::PreRender) etc. when registering systems
inline flecs::entity phase_pre_render;
inline flecs::entity phase_on_render;
inline flecs::entity phase_post_render;
inline flecs::entity phase_on_present;

// some internal state
namespace detail {
    inline bool draw_grid = true;
    inline bool draw_fps = true;
    inline int grid_slices = 12;
    inline float grid_spacing = 10.0f / 12.0f;
    inline Font font = {0};
    inline bool font_loaded = false;
} // namespace detail

// get the loaded font (falls back to default if not loaded)
[[nodiscard]] inline Font get_font() {
    return detail::font_loaded ? detail::font : GetFontDefault();
}

// configuration for window and rendering
struct WindowConfig {
    int width = 800;
    int height = 600;
    const char* title = "Graphics";
    int target_fps = 60;
    Color clear_color = RAYWHITE;
};

namespace detail {
    struct CanvasSize {
        int width = 0;
        int height = 0;
    };

    void finalize_world();
    void run_frame(ecs_ftime_t dt);
    void platform_before_init_window(const WindowConfig& config, CanvasSize& canvas_size);
    void platform_after_init_window(const WindowConfig& config, const CanvasSize& canvas_size);
    void platform_set_target_fps(const WindowConfig& config);
    void platform_pre_close_window();
    void platform_run_loop();
} // namespace detail

// check if we're on the render thread (for WASM threading safety) (so in worker, this always
// evaluates to false)
[[nodiscard]] inline bool is_render_thread() {
#ifdef __EMSCRIPTEN__
    return emscripten_is_main_browser_thread();
#else
    return true;
#endif
}

// initialize graphics module with flecs world.
// sets up rendering phases and core systems
void init(flecs::world& world);

// initialize window with config
void init_window(const WindowConfig& config);

// convenience overload
inline void init_window(int width, int height, const char* title) {
    init_window({width, height, title});
}

// run the main loop.
void run_loop();

// check if window should close
[[nodiscard]] bool window_should_close();

// close window and cleanup
void close_window();

// toggle grid rendering
inline void set_draw_grid(bool draw, int slices = 12, float spacing = 10.0f / 12.0f) {
    detail::draw_grid = draw;
    detail::grid_slices = slices;
    detail::grid_spacing = spacing;
}

// toggle FPS counter
inline void set_draw_fps(bool draw) {
    detail::draw_fps = draw;
}

// ============================================================================
// Camera helpers
// ============================================================================

// create a camera entity with optional activation.
// if make_active is true, this camera becomes the rendering camera.
flecs::entity create_camera(flecs::world& ecs, const char* name, const Camera& cam = {}, bool make_active = false);
flecs::entity create_camera(flecs::world& ecs, const char* name, const Position& pos, const Camera& cam = {}, bool make_active = false);

// switch the active camera for rendering
void set_active_camera(flecs::world &ecs, flecs::entity camera_entity);

// send an event from worker-hosted WASM to the main thread
inline void emit_worker_event(const char* name, const char* payload) {
#ifdef __EMSCRIPTEN__
    const char* safe_name = name ? name : "";
    const char* safe_payload = payload ? payload : "";
    EM_ASM({
        const eventName = UTF8ToString($0);
        const rawPayload = UTF8ToString($1);
        let data = null;
        if (rawPayload && rawPayload.length) {
            try {
                data = JSON.parse(rawPayload);
            } catch (err) {
                data = rawPayload;
            }
        }
        postMessage({ type: 'event', payload: { name: eventName, data } });
    }, safe_name, safe_payload);
#else
    (void)name;
    (void)payload;
#endif
}

} // namespace graphics
