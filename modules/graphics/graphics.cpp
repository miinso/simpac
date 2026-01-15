#include "graphics.h"

#include <cstdio>

#if defined(__EMSCRIPTEN__)
#    include <emscripten/emscripten.h>
#endif

namespace graphics {

namespace {
    // stored window config for clear color
    Color clear_color = RAYWHITE;

    // wasm shutdown safety flag - prevents use-after-free from pending RAF callbacks
    bool shutdown_requested = false;

    // setup the render pipeline systems
    void setup_render_pipeline() {
        auto* ecs = detail::ecs;

        // camera update in OnLoad (before rendering)
        // query for `Camera` with `ActiveCamera` tag
        ecs->system<Camera>("graphics::update_camera")
            .with<ActiveCamera>()
            .kind(flecs::OnLoad)
            .each([](Camera& cam) {
                update_camera_controls(cam);
            });

        // sync active camera to raylib before rendering
        ecs->system<const Camera>("graphics::sync_camera")
            .with<ActiveCamera>()
            .kind(flecs::PostUpdate)
            .each([](const Camera& cam) {
                detail::raylib_camera = to_raylib(cam);
            });

        // PreRender: begin drawing, clear, update lighting
        ecs->system("graphics::begin")
            .kind(phase_pre_render)
            .run([](flecs::iter& it) {
                if (!is_render_thread()) return;

                // handle canvas resize (web: JS ResizeObserver updates canvas.width/height)
                if (IsWindowResized()) {
                    SetWindowSize(GetScreenWidth(), GetScreenHeight());
                }

                BeginDrawing();
                ClearBackground(clear_color);

                // update lighting with camera position if enabled
                if (is_lighting_enabled()) {
                    update_lighting_camera_pos(detail::raylib_camera.position);
                }
            });

        // OnRender: begin 3D mode
        ecs->system("graphics::render3d")
            .kind(phase_on_render)
            .run([](flecs::iter& it) {
                if (!is_render_thread()) return;

                BeginMode3D(detail::raylib_camera);

                // enable shader mode if lighting is active
                if (is_lighting_enabled()) {
                    BeginShaderMode(get_lighting_shader());
                }
            });

        // PostRender: end 3D mode, draw overlays
        ecs->system("graphics::render2d")
            .kind(phase_post_render)
            .run([](flecs::iter& it) {
                if (!is_render_thread()) return;

                // end shader mode if lighting was active
                if (is_lighting_enabled()) {
                    EndShaderMode();
                }

                // draw grid if enabled
                if (detail::draw_grid) {
                    DrawGrid(detail::grid_slices, detail::grid_spacing);
                }

                EndMode3D();

                // draw FPS if enabled
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
    // emscripten main loop callback
    void main_loop_callback() {
        // bail early if shutdown was requested (handles pending RAF after cancel)
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

    // register camera components with reflection
    register_camera_components(world);

    // create custom rendering phases
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
    // read actual canvas size BEFORE InitWindow (which overwrites it)
    int canvas_w = EM_ASM_INT({ return Module.canvas.width; });
    int canvas_h = EM_ASM_INT({ return Module.canvas.height; });
    std::printf("graphics::init_window requested=%dx%d canvas=%dx%d\n",
                config.width, config.height, canvas_w, canvas_h);
#endif
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(config.width, config.height, config.title);

#if defined(__EMSCRIPTEN__)
    // restore actual canvas size if different from requested
    if (canvas_w > 0 && canvas_h > 0 &&
        (canvas_w != config.width || canvas_h != config.height)) {
        SetWindowSize(canvas_w, canvas_h);
    }
#endif

    clear_color = config.clear_color;

    // create default camera entity if none exists
    if (detail::ecs) {
        auto active_cam_query = detail::ecs->query<Camera, ActiveCamera>();
        if (active_cam_query.count() == 0) {
            detail::ecs->entity("DefaultCamera")
                .set<Camera>({})
                .add<ActiveCamera>();
        }
    }

    // load custom font
    std::string font_path = normalized_path("resources/generic.fnt");
    detail::font = LoadFont(font_path.c_str());
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
    // set flag FIRST so any pending RAF callback bails early
    shutdown_requested = true;

    // cleanup resources
    if (detail::ecs) {
        detail::ecs->progress(0);
    }

    emscripten_cancel_main_loop();
    std::printf("graphics::emscripten rAF loop canceled\n");
#endif

    // cleanup lighting if enabled
    disable_lighting();

    // unload custom font
    if (detail::font_loaded) {
        UnloadFont(detail::font);
        detail::font_loaded = false;
    }

    CloseWindow();
}

void run_main_loop(std::function<void()> update) {
    detail::update_func = update;

#if defined(__EMSCRIPTEN__)
    // reset shutdown flag for fresh start
    shutdown_requested = false;
    std::printf("graphics::invoking emscripten rAF loop\n");
    emscripten_set_main_loop(main_loop_callback, 0, 1);
#else
    std::printf("graphics::invoking native while loop\n");
    while (!window_should_close()) {
        // user update first (input handling before EndDrawing clears key state)
        if (detail::update_func) {
            detail::update_func();
        }

        // then ECS progress (rendering systems)
        if (detail::ecs) {
            detail::ecs->progress(GetFrameTime());
        }
    }
    std::printf("graphics::destroying native while loop\n");
    close_window();
#endif
}

// ============================================================================
// Camera helper functions
// ============================================================================

flecs::entity create_camera(flecs::world& ecs, const char* name, const Camera& cam, bool make_active) {
    auto entity = ecs.entity(name).set<Camera>(cam);

    if (make_active) {
        // remove ActiveCamera from any existing camera
        ecs.query_builder<Camera>()
            .with<ActiveCamera>()
            .build()
            .each([](flecs::entity e, Camera&) {
                e.remove<ActiveCamera>();
            });
        entity.add<ActiveCamera>();
    }

    return entity;
}

void set_active_camera(flecs::world& ecs, flecs::entity camera_entity) {
    // remove ActiveCamera from all cameras
    ecs.query_builder<Camera>()
        .with<ActiveCamera>()
        .build()
        .each([](flecs::entity e, Camera&) {
            e.remove<ActiveCamera>();
        });

    // add to the specified camera
    if (camera_entity.has<Camera>()) {
        camera_entity.add<ActiveCamera>();
    }
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
