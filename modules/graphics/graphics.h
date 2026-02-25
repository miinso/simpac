#pragma once

#include <flecs.h>
#include <raylib.h>
#include <raymath.h>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <emscripten/threading.h>
#endif


inline constexpr const char* GLSL_VERSION = "300es";

#include "input.h"

// sub-headers (shim enabled) (order matters)
#include "camera.h"
#include "components.h"
#include "fonts.h"
#include "resources.h"
#include "lights.h"
#include "lighting.h"
#include "materials.h"
#include "pipelines.h"
#include "systems.h"
#include "vars.h"

namespace graphics {

// get the loaded font (falls back to default if not loaded)
[[nodiscard]] inline Font get_font() {
    return fonts::get();
}

// get the loaded tiny font (3x5) (falls back to default if not loaded)
[[nodiscard]] inline Font get_font_tiny() {
    return fonts::get_tiny();
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
void init(flecs::world& world, const WindowConfig& config);

// run the main loop.
void run_loop();

// check if window should close
[[nodiscard]] bool window_should_close();

// close window and cleanup
void close_window();

// camera helpers live in `graphics::camera`.

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
