#pragma once

#include "../components.h"
#include "../queries.h"
#include "../vars.h"
#include "graphics.h"
#include "flecs.h"

#include <raylib.h>
#include <raymath.h>

#include <algorithm>
#include <cmath>
#include <string>

namespace render {

inline void draw_spring(Spring& spring) {
    auto x1 = spring.e1.get<Position>().map();
    auto x2 = spring.e2.get<Position>().map();

    auto diff = x1 - x2;
    auto current_length = diff.norm();
    auto strain = (current_length - spring.rest_length) / spring.rest_length;

    Color color;
    if (strain > 0) {
        float t = std::min(1.0f, (float)strain * 10.0f);
        color = ColorLerp(GREEN, RED, t);
    } else {
        float t = std::min(1.0f, (float)(-strain) * 10.0f);
        color = ColorLerp(GREEN, BLUE, t);
    }

    Vector3 a{x1[0], x1[1], x1[2]};
    Vector3 b{x2[0], x2[1], x2[2]};
    float base_thickness = 0.005f;
    float thickness = base_thickness * std::pow((float)spring.k_s, 1.0f / 3.0f);
    DrawCylinderEx(a, b, thickness, thickness, 5, color);
}

inline void draw_particle(const Position& x, const Mass& m) {
    Vector3 pos{x[0], x[1], x[2]};
    DrawPoint3D(pos, BLUE);
    DrawSphere(pos, 0.5, BLUE);
}

inline void draw_timing_info(flecs::iter& it) {
    const auto& wall_time = state::wall_time.get<Real>();
    const auto& sim_time = state::sim_time.get<Real>();
    const auto& sim_dt = props::dt.get<Real>();
    const auto& frame_count = state::frame_count.get<int>();
    const Solver* solver = it.world().try_get<Solver>();

    const Real display_dt = (Real)it.delta_time();
    Real realtime_speed = display_dt > Real(1e-9) ? sim_dt / display_dt : Real(0);

    Font font = graphics::get_font();
    float font_size = (float)font.baseSize;
    char buf[512];
    const char* cg_prefix = solver ? "" : "N/A ";
    int cg_iterations = solver ? solver->cg_iterations : 0;
    Real cg_error = solver ? solver->cg_error : Real(0);
    int cg_max_iter = solver ? it.world().lookup("Config::Solver::cg_max_iter").get<int>() : 0;
    Real cg_tolerance = solver ? it.world().lookup("Config::Solver::cg_tolerance").get<Real>() : Real(0);

    snprintf(buf, sizeof(buf),
        "Wall time: %.2fs  |  Sim time: %.2fs  (dt=%.4f)\n"
        "Frame: %d  |  Speed: %.2fx\n"
        "CG: %s%d/%d iter, tol=%.0e, err=%.2e\n"
        "\n"
        "Particles: %d  Springs: %d\n"
        "\n"
        "Strain: Red (tension), Green (rest), Blue (compression)\n"
        "Camera: WASDQE / Arrows / MouseDrag / MouseScroll",
        wall_time, sim_time, sim_dt,
        frame_count, realtime_speed,
        cg_prefix, cg_iterations, cg_max_iter, cg_tolerance, cg_error,
        queries::num_particles(), queries::num_springs());
    DrawTextEx(font, buf, {21, 41}, font_size, 0, WHITE);
    DrawTextEx(font, buf, {20, 40}, font_size, 0, DARKGRAY);
}

inline void draw_solve_history(flecs::iter& it) {
    auto& solver = it.world().get<Solver>();
    if (solver.cg_history.empty()) return;

    Font font = graphics::get_font();
    float font_size = (float)font.baseSize;

    std::string history_text;
    for (const auto& line : solver.cg_history) {
        history_text += line + "\n";
    }

    int screen_width = GetScreenWidth();
    int screen_height = GetScreenHeight();
    Vector2 text_size = MeasureTextEx(font, history_text.c_str(), font_size, 0);
    float x = screen_width - text_size.x - 20;
    float y = screen_height - text_size.y;
    DrawTextEx(font, history_text.c_str(), {x, y}, font_size, 0, BLACK);
}

} // namespace render

