#pragma once

#include "components.h"
#include "graphics.h"
#include "flecs.h"
#include <raylib.h>
#include <cmath>

namespace systems {

constexpr Real kEpsilon = 1e-6f;

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
    auto alpha = 1.0f / (c.stiffness * dt * dt);
    auto delta_lambda = -(C + alpha * c.lambda) / (w + alpha);

    c.lambda += delta_lambda;
    x1 += w1 * delta_lambda * grad;
    x2 -= w2 * delta_lambda * grad;
}

inline void ground_collision(Position& x, const OldPosition& x_old, Real dt, Real friction = 0.8f) {
    if (x.value.y() < 0) {
        x.value.y() = 0;
        auto displacement = x.value - x_old.value;
        auto friction_factor = std::min(1.0f, dt * friction);
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
    return {v.x(), v.y(), v.z()};
}

inline void draw_particle(const Position& x) {
    DrawPoint3D(toRay3(x.value), BLUE);
}

// =========================================================================
// Helper functions
// =========================================================================

inline flecs::entity add_particle(flecs::world& ecs, const Vector3r& pos, Real mass = 1.0f) {
    auto e = ecs.entity();
    e.add<Particle>()
        .set<Position>({pos})
        .set<OldPosition>({pos})
        .set<Velocity>({Vector3r::Zero()})
        .set<Acceleration>({Vector3r::Zero()})
        .set<Mass>({mass})
        .set<InverseMass>({1.0f / mass});
    return e;
}

inline flecs::entity add_distance_constraint(flecs::world& ecs, flecs::entity& e1, flecs::entity& e2, Real stiffness = 1.0f) {
    auto rest_distance = (e1.get<Position>().value - e2.get<Position>().value).norm();

    auto e = ecs.entity();
    e.add<Constraint>()
        .set<DistanceConstraint>({e1, e2, rest_distance, stiffness, 0});
    return e;
}

} // namespace systems
