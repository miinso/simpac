#include "graphics.h"

#include <cstdio>
#include <functional>

namespace graphics {

namespace {
    // stored window config for clear color
    Color clear_color = RAYWHITE;
    ecs_world_t* runtime_world = nullptr;
    std::function<void()> progress_func;

    struct AppLoopState {
        ecs_world_t* world = nullptr;
        ecs_app_desc_t desc = {};

        void reset_runtime() {
            world = nullptr;
            desc = {};
        }
    };

    AppLoopState app_loop{};

    // app loop hook for app().run
    void app_frame() {
        if (!app_loop.world) {
            return;
        }
        ecs_app_run_frame(app_loop.world, &app_loop.desc);
    }

    // app run hook: copy desc + run_loop
    int app_run_action(ecs_world_t* world, ecs_app_desc_t* desc) {
        app_loop.world = world;
        runtime_world = world;
        // keep stable copy; desc may be stack alloc
        app_loop.desc = *desc;
        progress_func = app_frame;
        run_loop();
        app_loop.reset_runtime();
        return 0;
    }

    void install_app_loop() {
        ecs_app_set_run_action(app_run_action);
    }

    // setup the render pipeline systems
    void setup_render_pipeline(flecs::world& ecs) {
        // camera update in OnLoad (before rendering)
        // query for `Camera` with `ActiveCamera` tag
        ecs.system<Position, Camera>("graphics::update_camera")
            .with<ActiveCamera>()
            .kind(flecs::OnLoad)
            .each([](Position& pos, Camera& cam) {
                update_camera_controls(pos, cam);
            });

        // sync active camera to raylib before rendering
        ecs.system<const Position, const Camera>("graphics::sync_camera")
            .with<ActiveCamera>()
            .kind(flecs::PostUpdate)
            .each([](const Position& pos, const Camera& cam) {
                detail::raylib_camera = to_raylib(pos, cam);
            });

        // PreRender: begin drawing, clear, update lighting
        ecs.system("graphics::begin")
            .kind(phase_pre_render)
            .run([](flecs::iter& it) {
                // update input shim state (history, deltas)
                graphics::shim::update();

                // if (!is_render_thread()) return;

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
        ecs.system("graphics::render3d")
            .kind(phase_on_render)
            .run([](flecs::iter& it) {
                // if (!is_render_thread()) return;

                BeginMode3D(detail::raylib_camera);

                // enable shader mode if lighting is active
                if (is_lighting_enabled()) {
                    BeginShaderMode(get_lighting_shader());
                }
            });

        // PostRender: end 3D mode, draw overlays
        ecs.system("graphics::render2d")
            .kind(phase_post_render)
            .run([](flecs::iter& it) {
                // if (!is_render_thread()) return;

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
        ecs.system("graphics::end")
            .kind(phase_on_present)
            .run([](flecs::iter& it) {
                // if (!is_render_thread()) return;
                EndDrawing();
            });
    }

} // anonymous namespace

namespace detail {
    void finalize_world() {
        ecs_world_t* world = runtime_world;
        if (!world) {
            return;
        }
        runtime_world = nullptr;
        progress_func = nullptr;
        if (!flecs_poly_release(world)) {
            if (ecs_stage_get_id(world) == -1) {
                ecs_stage_free(world);
            } else {
                // mirror flecs::world::release() to avoid recursive fini
                flecs_poly_claim(world);
                ecs_fini(world);
            }
        }
    }

    void run_frame(ecs_ftime_t dt) {
        if (progress_func) {
            progress_func();
        } else if (runtime_world) {
            ecs_progress(runtime_world, dt);
        }
    }
} // namespace detail

void init(flecs::world& world) {
    runtime_world = world.c_ptr();
    progress_func = nullptr;
    install_app_loop();

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

    setup_render_pipeline(world);
}

void init_window(const WindowConfig& config) {
    detail::CanvasSize canvas_size;
    detail::platform_before_init_window(config, canvas_size);

#if defined(__EMSCRIPTEN__)
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
#else
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
#endif
    InitWindow(config.width, config.height, config.title);

    detail::platform_after_init_window(config, canvas_size);

    clear_color = config.clear_color;

    // create default camera entity if none exists
    if (runtime_world) {
        flecs::world ecs(runtime_world);
        auto active_cam_query = ecs.query<Camera, ActiveCamera>();
        if (active_cam_query.count() == 0) {
            ecs.entity("DefaultCamera")
                .set<Camera>({})
                .set<Position>({})
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

    detail::platform_set_target_fps(config);
}

bool window_should_close() {
    return WindowShouldClose();
}

void close_window() {
    const bool window_ready = IsWindowReady();
    if (window_ready) {
        detail::platform_pre_close_window();

        // cleanup lighting if enabled
        disable_lighting();

        // unload custom font
        if (detail::font_loaded) {
            UnloadFont(detail::font);
            detail::font_loaded = false;
        }

        CloseWindow();
    }

    // clear global phase entities (they hold flecs refs)
    phase_pre_render = flecs::entity();
    phase_on_render = flecs::entity();
    phase_post_render = flecs::entity();
    phase_on_present = flecs::entity();

    // clear hooks (may capture refs)
    progress_func = nullptr;
    app_loop.reset_runtime();
    // world lifetime is owned by the caller; clear cached pointer only
    runtime_world = nullptr;
}

void run_loop() {
    detail::platform_run_loop();
}

// ============================================================================
// Camera helper functions
// ============================================================================

flecs::entity create_camera(flecs::world& ecs, const char* name, const Camera& cam, bool make_active) {
    return create_camera(ecs, name, Position{}, cam, make_active);
}

flecs::entity create_camera(flecs::world& ecs, const char* name, const Position& pos, const Camera& cam, bool make_active) {
    auto entity = ecs.entity(name)
                            .set<Camera>(cam)
                            .set<Position>(pos);

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
