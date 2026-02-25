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
    components::register_light_components(world);
    components::register_material_components(world);
    queries::seed(world);

    // renderer lifecycle hooks — init GPU resources on set, cleanup on remove
    world.component<LightRenderer>()
        .on_set([](flecs::entity, LightRenderer& lr) {
            lr.shader = LoadShader(lr.vs_path, lr.fs_path);
            lr.view_pos_loc = GetShaderLocation(lr.shader, "viewPos");
            lr.shader.locs[SHADER_LOC_VECTOR_VIEW] = lr.view_pos_loc;
            lr.ambient_color_loc = GetShaderLocation(lr.shader, "ambientColor");
            lr.ambient_intensity_loc = GetShaderLocation(lr.shader, "ambient");
            lr.num_lights_loc = GetShaderLocation(lr.shader, "numOfLights");
            for (int i = 0; i < MAX_LIGHTS; ++i) {
                auto& loc = lr.light_locs[i];
                loc.enabled   = GetShaderLocation(lr.shader, TextFormat("lights[%i].enabled", i));
                loc.type      = GetShaderLocation(lr.shader, TextFormat("lights[%i].type", i));
                loc.position  = GetShaderLocation(lr.shader, TextFormat("lights[%i].position", i));
                loc.direction = GetShaderLocation(lr.shader, TextFormat("lights[%i].direction", i));
                loc.color     = GetShaderLocation(lr.shader, TextFormat("lights[%i].color", i));
                loc.intensity = GetShaderLocation(lr.shader, TextFormat("lights[%i].intensity", i));
                loc.range     = GetShaderLocation(lr.shader, TextFormat("lights[%i].range", i));
                loc.falloff   = GetShaderLocation(lr.shader, TextFormat("lights[%i].falloff", i));
                loc.cos_inner = GetShaderLocation(lr.shader, TextFormat("lights[%i].cosInner", i));
                loc.cos_outer = GetShaderLocation(lr.shader, TextFormat("lights[%i].cosOuter", i));
            }
            lr.mat_locs = MaterialLocs::cache(lr.shader);
        })
        .on_remove([](flecs::entity, LightRenderer& lr) {
            if (lr.shader.id) {
                UnloadShader(lr.shader);
                lr.shader = {};
            }
        });

    // NOTE: requires LightRenderer set first; silently skips if missing
    world.component<ShadowRenderer>()
        .on_set([](flecs::entity e, ShadowRenderer& sr) {
            auto* lr = e.world().try_get<LightRenderer>();
            if (!lr || !lr->shader.id) return;

            // directional shadow FBO
            sr.fbo = rlLoadFramebuffer();
            if (!sr.fbo) return;
            sr.depth_tex = rlLoadTextureDepth(sr.resolution, sr.resolution, false);
            rlFramebufferAttach(sr.fbo, sr.depth_tex, RL_ATTACHMENT_DEPTH, RL_ATTACHMENT_TEXTURE2D, 0);
            if (!rlFramebufferComplete(sr.fbo)) {
                rlUnloadFramebuffer(sr.fbo);
                rlUnloadTexture(sr.depth_tex);
                sr.fbo = 0; sr.depth_tex = 0;
                return;
            }

            sr.depth_shader = LoadShader(
                npath(TextFormat("resources/shaders/glsl%s/depth.vs", GLSL_VERSION)).c_str(),
                npath(TextFormat("resources/shaders/glsl%s/depth.fs", GLSL_VERSION)).c_str());
            sr.depth_mvp_loc = GetShaderLocation(sr.depth_shader, "mvp");
            sr.depth_shader.locs[SHADER_LOC_MATRIX_MVP] = sr.depth_mvp_loc;
            sr.shadow_map_loc  = GetShaderLocation(lr->shader, "shadowMap");
            sr.light_vp_loc    = GetShaderLocation(lr->shader, "lightVP");
            sr.shadow_bias_loc = GetShaderLocation(lr->shader, "shadowBias");

            // spot shadow FBO
            sr.spot_fbo = rlLoadFramebuffer();
            if (!sr.spot_fbo) return;
            sr.spot_depth_tex = rlLoadTextureDepth(sr.spot_resolution, sr.spot_resolution, false);
            rlFramebufferAttach(sr.spot_fbo, sr.spot_depth_tex, RL_ATTACHMENT_DEPTH, RL_ATTACHMENT_TEXTURE2D, 0);
            if (!rlFramebufferComplete(sr.spot_fbo)) {
                rlUnloadFramebuffer(sr.spot_fbo);
                rlUnloadTexture(sr.spot_depth_tex);
                sr.spot_fbo = 0; sr.spot_depth_tex = 0;
                return;
            }
            sr.spot_shadow_map_loc  = GetShaderLocation(lr->shader, "shadowMapSpot");
            sr.spot_light_vp_loc    = GetShaderLocation(lr->shader, "lightVPSpot");
            sr.spot_shadow_bias_loc = GetShaderLocation(lr->shader, "shadowBiasSpot");
        })
        .on_remove([](flecs::entity, ShadowRenderer& sr) {
            if (sr.spot_fbo) {
                rlUnloadFramebuffer(sr.spot_fbo);
                rlUnloadTexture(sr.spot_depth_tex);
                sr.spot_fbo = 0; sr.spot_depth_tex = 0;
            }
            if (sr.fbo) {
                UnloadShader(sr.depth_shader);
                rlUnloadFramebuffer(sr.fbo);
                rlUnloadTexture(sr.depth_tex);
                sr.depth_shader = {};
                sr.fbo = 0; sr.depth_tex = 0;
            }
        });

    props::seed(world);
    PreRender = world.entity("graphics::PreRender")
        .add(flecs::Phase)
        .depends_on(flecs::PostUpdate);
    OnShadowPass = world.entity("graphics::OnShadowPass")
        .add(flecs::Phase)
        .depends_on(PreRender);
    OnRender = world.entity("graphics::OnRender")
        .add(flecs::Phase)
        .depends_on(OnShadowPass);
    OnRenderOverlay = world.entity("graphics::OnRenderOverlay")
        .add(flecs::Phase)
        .depends_on(OnRender);
    PostRender = world.entity("graphics::PostRender")
        .add(flecs::Phase)
        .depends_on(OnRenderOverlay);
    OnPresent = world.entity("graphics::OnPresent")
        .add(flecs::Phase)
        .depends_on(PostRender);
    systems::install_pipeline_systems(world);
}

void init(flecs::world& world, const WindowConfig& config) {
    init(world);
    init_window(config);
}

bool window_should_close() {
    return WindowShouldClose();
}

void close_window() {
    const bool window_ready = IsWindowReady();
    if (window_ready) {
        detail::platform_pre_close_window();

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
