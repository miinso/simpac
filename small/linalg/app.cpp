#include "app.h"

#ifdef EMSCRIPTEN
#    include <emscripten/emscripten.h>
#endif

#include <flecs.h>
#include <raylib.h>
#include <utility>

#include "modules/engine/core/components.h"
#include "modules/engine/core/core_module.h"
#include "modules/engine/rendering/components.h"
#include "modules/engine/rendering/rendering_module.h"

App::App(std::string window_name, int window_width, int window_height)
        : m_world{},
            m_window_name(std::move(window_name)),
      m_window_width(window_width),
      m_window_height(window_height) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(m_window_width, m_window_height, m_window_name.c_str());
    m_window_initialized = true;

    m_world.import<core::CoreModule>();
    m_world.import<rendering::RenderingModule>();

    m_world.set<core::Settings>(
        {m_window_name, m_window_width, m_window_height, m_window_width, m_window_height});

    auto camera_target = m_world.entity("camera_target").set<core::Position>(
        {Eigen::Vector2f::Zero()});

    rendering::TrackingCamera camera{};
    camera.target = camera_target;
    camera.camera.target = {0.0f, 0.0f};
    camera.camera.offset = {
        static_cast<float>(m_window_width) * 0.5f,
        static_cast<float>(m_window_height) * 0.5f,
    };
    camera.camera.zoom = 1.0f;
    camera.camera.rotation = 0.0f;
    m_world.set<rendering::TrackingCamera>(camera);
}

App::~App() {
    if (m_window_initialized) {
        CloseWindow();
        m_window_initialized = false;
    }
}

void App::run() {
#ifdef EMSCRIPTEN
    emscripten_set_main_loop_arg(update_draw_frame_browser, this, 0, 1);
#else
    while (!WindowShouldClose()) {
        tick();
    }
#endif
}

void App::tick() {
    m_window_width = GetRenderWidth();
    m_window_height = GetRenderHeight();

    auto &settings = m_world.get_mut<core::Settings>();
    settings.window_width = m_window_width;
    settings.window_height = m_window_height;
    m_world.modified<core::Settings>();

    m_world.progress(GetFrameTime());
}

void App::update_draw_frame_browser(void *app) {
    auto *instance = static_cast<App *>(app);
    instance->tick();
}
