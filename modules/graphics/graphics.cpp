#include "graphics.h"

#include <cstdio>

#if defined(__EMSCRIPTEN__)
#    include <emscripten/emscripten.h>
#endif

namespace graphics {

namespace {
    // Stored window config for clear color
    Color clear_color = RAYWHITE;

    // WASM shutdown safety flag - prevents use-after-free from pending RAF callbacks
    bool shutdown_requested = false;

    // Setup the render pipeline systems
    void setup_render_pipeline() {
        auto* ecs = detail::ecs;

        // Camera update in OnLoad (before rendering)
        ecs->system("graphics::update_camera")
            .kind(flecs::OnLoad)
            .run([](flecs::iter& it) {
                update_camera_controls();
            });

        // PreRender: begin drawing, clear, update lighting
        ecs->system("graphics::begin")
            .kind(phase_pre_render)
            .run([](flecs::iter& it) {
                if (!is_render_thread()) return;

                // Handle canvas resize (web: JS ResizeObserver updates canvas.width/height)
                if (IsWindowResized()) {
                    SetWindowSize(GetScreenWidth(), GetScreenHeight());
                }

                BeginDrawing();
                ClearBackground(clear_color);

                // Update lighting with camera position if enabled
                if (is_lighting_enabled()) {
                    update_lighting_camera_pos(detail::camera.position);
                }
            });

        // OnRender: begin 3D mode
        ecs->system("graphics::render3d")
            .kind(phase_on_render)
            .run([](flecs::iter& it) {
                if (!is_render_thread()) return;

                BeginMode3D(detail::camera);

                // Enable shader mode if lighting is active
                if (is_lighting_enabled()) {
                    BeginShaderMode(get_lighting_shader());
                }
            });

        // PostRender: end 3D mode, draw overlays
        ecs->system("graphics::render2d")
            .kind(phase_post_render)
            .run([](flecs::iter& it) {
                if (!is_render_thread()) return;

                // End shader mode if lighting was active
                if (is_lighting_enabled()) {
                    EndShaderMode();
                }

                // Draw grid if enabled
                if (detail::draw_grid) {
                    DrawGrid(detail::grid_slices, detail::grid_spacing);
                }

                EndMode3D();

                // Draw FPS if enabled
                if (detail::draw_fps) {
                    DrawFPS(20, 20);
                }
            });

        // OnPresent: end drawing
        ecs->system("graphics::end")
            .kind(phase_on_present)
            .run([](flecs::iter& it) {
                if (!is_render_thread()) return;
                EndDrawing();
            });
    }

#if defined(__EMSCRIPTEN__)
    // Emscripten main loop callback
    void main_loop_callback() {
        // Bail early if shutdown was requested (handles pending RAF after cancel)
        if (shutdown_requested) return;

        if (detail::ecs) {
            detail::ecs->progress();
        }

        if (detail::update_func) {
            detail::update_func();
        }
    }
#endif

} // anonymous namespace

void init(flecs::world& world) {
    detail::ecs = &world;

    // Create custom rendering phases
    phase_pre_render = world.entity("PreRender")
        .add(flecs::Phase)
        .depends_on(flecs::PostUpdate);

    phase_on_render = world.entity("OnRender")
        .add(flecs::Phase)
        .depends_on(phase_pre_render);

    phase_post_render = world.entity("PostRender")
        .add(flecs::Phase)
        .depends_on(phase_on_render);

    phase_on_present = world.entity("OnPresent")
        .add(flecs::Phase)
        .depends_on(phase_post_render);

    setup_render_pipeline();
}

void init_window(const WindowConfig& config) {
#if defined(__EMSCRIPTEN__)
    // Read actual canvas size BEFORE InitWindow (which overwrites it)
    int canvas_w = EM_ASM_INT({ return Module.canvas.width; });
    int canvas_h = EM_ASM_INT({ return Module.canvas.height; });
    std::printf("graphics::init_window requested=%dx%d canvas=%dx%d\n",
                config.width, config.height, canvas_w, canvas_h);
#endif

    InitWindow(config.width, config.height, config.title);

#if defined(__EMSCRIPTEN__)
    // Restore actual canvas size if different from requested
    if (canvas_w > 0 && canvas_h > 0 &&
        (canvas_w != config.width || canvas_h != config.height)) {
        SetWindowSize(canvas_w, canvas_h);
    }
#endif

    clear_color = config.clear_color;
    init_camera();

    // Load custom font using resolved path
    // NOTE: init_resource_paths(argv[0]) must be called before this!
    const char* font_path = resolve_resource_path("resources/generic.fnt");
    detail::font = LoadFont(font_path);
    if (detail::font.texture.id > 0) {
        detail::font_loaded = true;
        // SetTextureFilter(detail::font.texture, TEXTURE_FILTER_POINT);
    }

#if !defined(__EMSCRIPTEN__)
    SetTargetFPS(config.target_fps);
#endif
}

bool window_should_close() {
    return WindowShouldClose();
}

void close_window() {
#if defined(__EMSCRIPTEN__)
    // Set flag FIRST so any pending RAF callback bails early
    shutdown_requested = true;

    // Cleanup resources
    if (detail::ecs) {
        detail::ecs->progress(0);
    }

    emscripten_cancel_main_loop();
    std::printf("graphics::emscripten rAF loop canceled\n");
#endif

    // Cleanup lighting if enabled
    disable_lighting();

    // Unload custom font
    if (detail::font_loaded) {
        UnloadFont(detail::font);
        detail::font_loaded = false;
    }

    CloseWindow();
}

void run_main_loop(std::function<void()> update) {
    detail::update_func = update;

#if defined(__EMSCRIPTEN__)
    // Reset shutdown flag for fresh start
    shutdown_requested = false;
    std::printf("graphics::invoking emscripten rAF loop\n");
    emscripten_set_main_loop(main_loop_callback, 0, 1);
#else
    std::printf("graphics::invoking native while loop\n");
    while (!window_should_close()) {
        // User update first (input handling before EndDrawing clears key state)
        if (detail::update_func) {
            detail::update_func();
        }

        // Then ECS progress (rendering systems)
        if (detail::ecs) {
            detail::ecs->progress(GetFrameTime());
        }
    }
    std::printf("graphics::destroying native while loop\n");
    close_window();
#endif
}

} // namespace graphics

#if defined(__EMSCRIPTEN__)
extern "C" {
    EMSCRIPTEN_KEEPALIVE void emscripten_shutdown() {
        graphics::close_window();
    }

    EMSCRIPTEN_KEEPALIVE void emscripten_resize(int width, int height) {
        std::printf("graphics::emscripten_resize %dx%d\n", width, height);
        SetWindowSize(width, height);
    }
}
#endif
