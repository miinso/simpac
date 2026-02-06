#include "graphics.h"

#include <cstdio>
#include <functional>
#include <algorithm>

namespace graphics {

void ColorRGBA::meta(flecs::world& ecs) {
    ecs.component<ColorRGBA>()
        .member("r", &ColorRGBA::r)
        .member("g", &ColorRGBA::g)
        .member("b", &ColorRGBA::b)
        .member("a", &ColorRGBA::a);
}

namespace {
    ecs_world_t* runtime_world = nullptr;
    std::function<void()> progress_func;

    ColorRGBA to_rgba(Color color) {
        return {
            color.r / 255.0f,
            color.g / 255.0f,
            color.b / 255.0f,
            color.a / 255.0f
        };
    }

    Color to_color(const ColorRGBA& color) {
        auto to_byte = [](float v) -> unsigned char {
            float clamped = std::clamp(v, 0.0f, 1.0f);
            return (unsigned char)(clamped * 255.0f);
        };
        return {
            to_byte(color.r),
            to_byte(color.g),
            to_byte(color.b),
            to_byte(color.a)
        };
    }

    Font load_font_grid(const char* path,
                        int cell_w,
                        int cell_h,
                        int spacing_x,
                        int spacing_y,
                        int pad_x,
                        int pad_y,
                        int first_char,
                        int overlap,
                        int space_advance) {
        Font font = {0};
        Image image = LoadImage(path);
        if (!image.data) {
            return font;
        }

        Color* pixels = LoadImageColors(image);
        if (!pixels) {
            UnloadImage(image);
            return font;
        }

        int cols = (image.width - pad_x) / (cell_w + spacing_x);
        int rows = (image.height - pad_y) / (cell_h + spacing_y);
        int count = cols * rows;
        if (cols <= 0 || rows <= 0 || count <= 0) {
            UnloadImageColors(pixels);
            UnloadImage(image);
            return font;
        }

        font.texture = LoadTextureFromImage(image);
        if (font.texture.id == 0) {
            UnloadImageColors(pixels);
            UnloadImage(image);
            return font;
        }

        font.glyphCount = count;
        font.glyphPadding = 0;
        font.recs = (Rectangle*)MemAlloc(sizeof(Rectangle) * count);
        font.glyphs = (GlyphInfo*)MemAlloc(sizeof(GlyphInfo) * count);
        if (space_advance < 1) {
            space_advance = 1;
        }
        if (overlap < 0) {
            overlap = 0;
        }
        for (int i = 0; i < count; ++i) {
            int col = i % cols;
            int row = i / cols;
            int cell_x = pad_x + col * (cell_w + spacing_x);
            int cell_y = pad_y + row * (cell_h + spacing_y);
            int left = cell_w;
            int right = -1;
            for (int y = 0; y < cell_h; ++y) {
                int row_offset = (cell_y + y) * image.width + cell_x;
                for (int x = 0; x < cell_w; ++x) {
                    if (pixels[row_offset + x].a > 0) {
                        if (x < left) left = x;
                        if (x > right) right = x;
                    }
                }
            }
            int rec_w = 0;
            if (right >= left) {
                rec_w = right - left + 1;
                font.recs[i] = Rectangle{
                    (float)(cell_x + left),
                    (float)cell_y,
                    (float)rec_w,
                    (float)cell_h
                };
            } else {
                font.recs[i] = Rectangle{(float)cell_x, (float)cell_y, 0.0f, (float)cell_h};
            }
            font.glyphs[i].value = first_char + i;
            font.glyphs[i].offsetX = 0;
            font.glyphs[i].offsetY = 0;
            if (rec_w > 0) {
                int advance = rec_w - overlap;
                if (advance < 1) advance = 1;
                font.glyphs[i].advanceX = advance;
            } else {
                font.glyphs[i].advanceX = space_advance;
            }
            font.glyphs[i].image = Image{};
        }

        int digit_start = 48 - first_char;
        int digit_end = 57 - first_char;
        if (digit_start >= 0 && digit_end < count) {
            int max_advance = 0;
            for (int i = digit_start; i <= digit_end; ++i) {
                if (font.glyphs[i].advanceX > max_advance) {
                    max_advance = font.glyphs[i].advanceX;
                }
            }
            if (max_advance > 0) {
                for (int i = digit_start; i <= digit_end; ++i) {
                    font.glyphs[i].advanceX = max_advance;
                }
            }
        }

        font.baseSize = cell_h;
        UnloadImageColors(pixels);
        UnloadImage(image);
        TraceLog(LOG_INFO, "FONT: [%s] Grid font loaded (%i pixel size | %i glyphs)",
                 path, font.baseSize, font.glyphCount);
        return font;
    }

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
                const auto& bg = it.world().lookup("Config::graphics::backgroundColor").get<ColorRGBA>();
                ClearBackground(to_color(bg));

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

                // FIXME: these will panic if lookup returns null
                const auto& show_grid = it.world().lookup("Config::graphics::debugOptions::showGrid").get<bool>();
                const auto& grid_slices = it.world().lookup("Config::graphics::debugOptions::gridSlices").get<int>();
                const auto& grid_spacing = it.world().lookup("Config::graphics::debugOptions::gridSpacing").get<float>();
                const auto& shows_stats = it.world().lookup("Config::graphics::showsStatistics").get<bool>();

                // draw grid if enabled
                if (show_grid) {
                    DrawGrid(grid_slices, grid_spacing);
                }

                EndMode3D();

                // draw FPS if enabled
                if (shows_stats) {
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
    ColorRGBA::meta(world);

    world.component<Configurable>();
    world.entity("Config::graphics::showsStatistics")
        .set<bool>(true)
        .add<Configurable>();
    world.entity("Config::graphics::debugOptions::showGrid")
        .set<bool>(true)
        .add<Configurable>();
    world.entity("Config::graphics::debugOptions::gridSlices")
        .set<int>(12)
        .add<Configurable>();
    world.entity("Config::graphics::debugOptions::gridSpacing")
        .set<float>(10.0f / 12.0f)
        .add<Configurable>();
    world.entity("Config::graphics::backgroundColor")
        .set<ColorRGBA>(to_rgba(RAYWHITE))
        .add<Configurable>();

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
    if (runtime_world) {
        flecs::world ecs(runtime_world);
        ecs.entity("Config::graphics::backgroundColor").set<ColorRGBA>(to_rgba(config.clear_color));
    }

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
    std::string font_path = normalized_path("resources/fonts/generic.fnt");
    detail::font = LoadFont(font_path.c_str());
    // SetTextureFilter(detail::font.texture, TEXTURE_FILTER_POINT);

    // load tiny 3x5 font
    std::string font_tiny_path = normalized_path("resources/fonts/tiny.png");
    detail::font_tiny = load_font_grid(font_tiny_path.c_str(), 5, 7, 0, 0, 0, 0, 0, 1, 3);
    if (detail::font_tiny.texture.id > 0 && detail::font_tiny.glyphCount > 0) {
        SetTextureFilter(detail::font_tiny.texture, TEXTURE_FILTER_POINT);
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
        if (detail::font.texture.id > 0) {
            UnloadFont(detail::font);
            detail::font = Font{};
        }

        // unload tiny font
        if (detail::font_tiny.texture.id > 0) {
            UnloadFont(detail::font_tiny);
            detail::font_tiny = Font{};
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
    // add to the specified camera
    if (camera_entity.has<Camera>()) {
        // remove ActiveCamera from all cameras
        ecs.query_builder<Camera>()
            .with<ActiveCamera>()
            .build()
            .each([](flecs::entity e, Camera&) {
                e.remove<ActiveCamera>();
            });
        camera_entity.add<ActiveCamera>();
    }
}

} // namespace graphics
