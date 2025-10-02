#include "app.h"

#ifdef EMSCRIPTEN
#include <emscripten/emscripten.h>
#endif

#include <flecs.h>
#include <raylib.h>

App::App(const char *window_name, int window_width, int window_height) :
    m_world(flecs::world()), m_window_name(window_name), m_window_width(window_width),
    m_window_height(window_height) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(m_window_width, m_window_height, m_window_name.c_str());
}

void App::run() {
    m_world.progress();
#ifdef EMSCRIPTEN
    emscripten_set_main_loop_arg(update_draw_frame_browser, this, 0, 1); // `this` == App
#else
    while (!WindowShouldClose()) {
        update_draw_frame_desktop();
    }
#endif
    CloseWindow();
}

void App::update_draw_frame_desktop() { m_world.progress(GetFrameTime()); }
void App::update_draw_frame_browser(void *app) {
    App *instance = static_cast<App *>(app);
    instance->m_world.progress(GetFrameTime());
}
