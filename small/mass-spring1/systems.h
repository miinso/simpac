#pragma once

#include "components.h"
#include "graphics.h"
#include "flecs.h"
#include <raylib.h>
#include <cmath>

namespace systems {

constexpr double kEpsilon = 1e-6;

// =========================================================================
// Simulation kernels
// =========================================================================

inline void clear_acceleration(Acceleration& a, const Vector3r& gravity) {
    a.value = gravity;
}

inline void clear_lambda(DistanceConstraint& c) {
    c.lambda = 0;
}

inline void integrate(Position& x, OldPosition& x_old, Velocity& v,
                      const Mass& mass, const Acceleration& acc, Real dt) {
    x_old.value = x.value;
    v.value += dt * acc.value;
    x.value += dt * v.value;
}

inline void solve_constraint(DistanceConstraint& c, Real dt) {
    auto& x1 = c.e1.get_mut<Position>().value;
    auto& x2 = c.e2.get_mut<Position>().value;
    auto w1 = c.e1.get<InverseMass>().value;
    auto w2 = c.e2.get<InverseMass>().value;

    auto w = w1 + w2;
    if (w < kEpsilon) return;

    auto delta = x1 - x2;
    auto distance = delta.norm();
    if (distance < kEpsilon) return;

    auto grad = delta / distance;
    auto C = distance - c.rest_distance;
    auto alpha = 1.0 / (c.stiffness * dt * dt);
    auto delta_lambda = -(C + alpha * c.lambda) / (w + alpha);

    c.lambda += delta_lambda;
    x1 += w1 * delta_lambda * grad;
    x2 -= w2 * delta_lambda * grad;
}

inline void ground_collision(Position& x, const OldPosition& x_old, Real dt, Real friction = 0.8) {
    if (x.value.y() < 0) {
        x.value.y() = 0;
        auto displacement = x.value - x_old.value;
        auto friction_factor = std::min(1.0, dt * friction);
        x.value.x() = x_old.value.x() + displacement.x() * (1 - friction_factor);
        x.value.z() = x_old.value.z() + displacement.z() * (1 - friction_factor);
    }
}

inline void update_velocity(Velocity& v, const Position& x, const OldPosition& x_old, Real dt) {
    v.value = (x.value - x_old.value) / dt;
}

// =========================================================================
// Drawing
// =========================================================================

inline Vector3 toRay3(const Vector3r& v) {
    return {(float)v.x(), (float)v.y(), (float)v.z()};
}

inline void draw_constraint(DistanceConstraint& c) {
    auto& x1 = c.e1.get_mut<Position>().value;
    auto& x2 = c.e2.get_mut<Position>().value;

    float base_thickness = 0.0005f;
    float thickness = base_thickness * std::pow((float)c.stiffness, 1.0f / 3.0f);

    float lambda_abs = std::abs((float)c.lambda);
    float t = lambda_abs > 1e-10f ? std::min(1.0f, -std::log10(lambda_abs) / 10.0f) : 1.0f;

    Color color = ColorLerp(RED, GREEN, t);
    DrawCylinderEx(toRay3(x1), toRay3(x2), thickness, thickness, 5, color);
}

inline void draw_particle(const Position& x, const Mass& m) {
    DrawSphere(toRay3(x.value), 0.05f * std::pow((float)m.value, 1.0f / 3.0f), BLUE);
}

inline void draw_mass_info(const Position& x, const Mass& m) {
    Vector3 textPos = toRay3(x.value);
    textPos.y += 0.2f;
    Vector2 screenPos = GetWorldToScreen(textPos, graphics::get_raylib_camera_const());

    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%0.2f", m.value);
    DrawText(buffer, (int)screenPos.x, (int)screenPos.y, 10, BLUE);
}

inline void draw_constraint_lambda(DistanceConstraint& c) {
    auto& x1 = c.e1.get_mut<Position>().value;
    auto& x2 = c.e2.get_mut<Position>().value;
    Vector3r midpoint = (x1 + x2) / 2.0;

    Vector3 textPos = toRay3(midpoint);
    Vector2 screenPos = GetWorldToScreen(textPos, graphics::get_raylib_camera_const());

    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%0.6f", c.lambda);
    DrawText(buffer, (int)screenPos.x, (int)screenPos.y, 10, BLUE);
}

inline void draw_timing_info(flecs::iter& it) {
    auto& scene = it.world().get_mut<Scene>();
    scene.wall_time += it.delta_time();

    Font font = graphics::get_font();
    char buf[512];
    snprintf(buf, sizeof(buf),
        "Wall time: %.2fs  |  Sim time: %.2fs  (dt=%.4f)\n"
        "Frame: %d  |  Speed: %.2fx\n"
        "Substeps: %d  |  Solver iter: %d\n"
        "\n"
        "Random cloth config each run\n"
        "Lambda: red(high) to green(low)\n"
        "Camera: WASDQE / Arrows / MouseDrag / MouseScroll",
        scene.wall_time, scene.sim_time, scene.dt,
        scene.frame_count, scene.sim_time / (scene.wall_time + 1e-9),
        scene.num_substeps, scene.solve_iter);
    DrawTextEx(font, buf, {21, 41}, 12, 0, WHITE);
    DrawTextEx(font, buf, {20, 40}, 12, 0, DARKGRAY);
}

} // namespace systems
