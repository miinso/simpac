#include "app.h"

#ifdef EMSCRIPTEN
#include <emscripten/emscripten.h>
#endif

#include "modules/engine/core/components.h"
#include "modules/engine/core/core_module.h"
#include "modules/engine/physics/physics_module.h"
#include "modules/engine/rendering/components.h"
#include "modules/engine/rendering/rendering_module.h"

#include <flecs.h>
#include <raylib.h>

using namespace core;
using namespace Eigen;

App::App(const char *window_name, int window_width, int window_height) :
    m_world(flecs::world()), m_window_name(window_name), m_window_width(window_width),
    m_window_height(window_height) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(m_window_width, m_window_height, m_window_name.c_str());

#ifndef EMSCRIPTEN
    m_world.import <flecs::units>();
    m_world.import <flecs::stats>();
    m_world.set<flecs::Rest>({/* specify port if needed */});
#endif
    m_world.import <core::CoreModule>();
    m_world.import <rendering::RenderingModule>();
    m_world.import <physics::PhysicsModule>();

    m_world.set<core::Settings>(
            {m_window_name, m_window_width, m_window_height, m_window_width, m_window_height});

    auto camera_lookat = m_world.entity("camera lookat").set<Position>({Vector2f(0, 0)});
    m_world.set<rendering::TrackingCamera>({camera_lookat, 0, 0});

    auto particle_prefab = m_world.prefab("particle")
                                   .set<particle_q>({Vector2f::Zero()})
                                   .set<particle_qd>({Vector2f::Zero()})
                                   .set<particle_f>({Vector2f::Zero()})
                                   .set<particle_inv_mass>({1})
                                   .add<PARTICLE_FLAG_ACTIVE>();

    for (int i = 0; i < 100; ++i) {
        m_world.entity()
                .is_a(particle_prefab)
                .set<particle_q>({Vector2f::Random() * 100})
                .set<particle_qd>({Vector2f::Random() * 10});
    }


    m_world.set<gravity>({Eigen::Vector2f(0, -10)});
    m_world.set<v_max>({1000});
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
