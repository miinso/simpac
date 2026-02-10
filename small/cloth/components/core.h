#pragma once

#include "base.h"

#include <cstdint>

struct Position : vec3<Position> {
    using vec3<Position>::vec3;
};

struct Velocity : vec3<Velocity> {
    using vec3<Velocity>::vec3;
};

struct Acceleration : vec3<Acceleration> {
    using vec3<Acceleration>::vec3;
};

struct Force : vec3<Force> {
    using vec3<Force>::vec3;
};

struct OldPosition : vec3<OldPosition> {
    using vec3<OldPosition>::vec3;
};

struct vec3f : vec3<vec3f> {
    using vec3<vec3f>::vec3;
};

using Gravity = vec3f;

struct Mass : scalar<Mass, Real> {
    using scalar<Mass, Real>::scalar;
};

struct InverseMass : scalar<InverseMass, Real> {
    using scalar<InverseMass, Real>::scalar;
};

struct ParticleIndex : scalar<ParticleIndex, int> {
    using scalar<ParticleIndex, int>::scalar;
};

struct ParticleState {
    enum Flag {
        None = 0,
        Hovered = 1 << 0,
        Selected = 1 << 1,
        Disabled = 1 << 2,
        Pinned = 1 << 3,
    };

    uint32_t flags = None;

    static void meta(flecs::world& ecs) {
        ecs.component<ParticleState>()
            .bit("Hovered", (uint32_t)ParticleState::Hovered)
            .bit("Selected", (uint32_t)ParticleState::Selected)
            .bit("Disabled", (uint32_t)ParticleState::Disabled)
            .bit("Pinned", (uint32_t)ParticleState::Pinned);
    }
};

// Tags
struct IsPinned {};
struct Particle {};
struct Constraint {};

struct DistanceConstraint {
    flecs::entity e1;
    flecs::entity e2;
    Real rest_distance;
    Real stiffness;
    Real lambda = 0;

    static void meta(flecs::world& ecs) {
        ecs.component<DistanceConstraint>()
            .member("e1", &DistanceConstraint::e1)
            .member("e2", &DistanceConstraint::e2)
            .member("rest_distance", &DistanceConstraint::rest_distance)
            .member("stiffness", &DistanceConstraint::stiffness)
            .member("lambda", &DistanceConstraint::lambda);
    }
};

struct Spring {
    flecs::entity e1;
    flecs::entity e2;
    Real rest_length = 0;
    Real k_s = 0;
    Real k_d = 0;

    static void meta(flecs::world& ecs) {
        ecs.component<Spring>()
            .member("e1", &Spring::e1)
            .member("e2", &Spring::e2)
            .member("rest_length", &Spring::rest_length)
                .range(0.0, 10.0)
            .member("k_s", &Spring::k_s)
                .range(0.0, 200000.0)
            .member("k_d", &Spring::k_d)
                .range(0.0, 10.0);
    }
};

struct Triangle {
    flecs::entity e1;
    flecs::entity e2;
    flecs::entity e3;
    Eigen::Matrix2r dm_inv = Eigen::Matrix2r::Zero();
    Real area = 0;
    Real thickness = 1;
    Real mu = 0;
    Real lambda = 0;
};

namespace components {

inline void register_core_components(flecs::world& ecs) {
    register_vec3<vec3f>(ecs);
    register_vec3<Position>(ecs);
    register_vec3<Velocity>(ecs);
    register_vec3<Acceleration>(ecs);
    register_vec3<Force>(ecs);
    register_vec3<OldPosition>(ecs);

    register_scalar<Mass>(ecs, flecs::F32);
    register_scalar<InverseMass>(ecs, flecs::F32);
    register_scalar<ParticleIndex>(ecs, flecs::I32);

    ParticleState::meta(ecs);
    Spring::meta(ecs);
    DistanceConstraint::meta(ecs);
    ecs.component<Triangle>();
}

} // namespace components
