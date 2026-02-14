#include "graphics.h"

#include <cstdio>
#include <functional>

namespace graphics {

namespace {
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

    components::register_camera_components(world);
    components::register_components(world);
    props::seed(world);
    PreRender = world.entity("graphics::PreRender")
        .add(flecs::Phase)
        .depends_on(flecs::PostUpdate);
    OnRender = world.entity("graphics::OnRender")
        .add(flecs::Phase)
        .depends_on(PreRender);
    PostRender = world.entity("graphics::PostRender")
        .add(flecs::Phase)
        .depends_on(OnRender);
    OnPresent = world.entity("graphics::OnPresent")
        .add(flecs::Phase)
        .depends_on(PostRender);
    systems::install_pipeline_systems(world);
}

void init(flecs::world& world, const WindowConfig& config) {
    init(world);
    init_window(config);
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
    if (runtime_world) {
        flecs::world ecs(runtime_world);
        props::background_color.set<color4f>(to_rgba(config.clear_color));

        auto default_camera = ecs.entity("DefaultCamera")
            .set<Camera>({})
            .set<Position>({});
        camera::active_camera_source(ecs).add<ActiveCamera>(default_camera.id());
    }

    fonts::load_from_resources();

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

        fonts::unload();

        CloseWindow();
    }

    // clear hooks (may capture refs)
    progress_func = nullptr;
    app_loop.reset_runtime();
    // world lifetime is owned by the caller; clear cached pointer only
    runtime_world = nullptr;
}

void run_loop() {
    detail::platform_run_loop();
}

} // namespace graphics
