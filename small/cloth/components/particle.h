#pragma once

#include <Eigen/Sparse>
#include <flecs.h>
#include "types.h"
#include "real.h"

#include <cstdint>
#include <cstddef>
#include <type_traits>

template <typename Derived>
using vec3 = graphics::vec3<Derived, Real>;

template <typename Derived>
using vec4 = graphics::vec4<Derived, Real>;

template <typename Derived, typename Value>
struct scalar {
    using value_type = Value;

    Value value = Value(0);

    scalar() = default;
    scalar(Value v) : value(v) {}

    scalar& operator=(Value v) {
        value = v;
        return *this;
    }

    operator Value&() { return value; }
    operator const Value&() const { return value; }
};

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

using vec3f = graphics::vec3f;
using vec3d = graphics::vec3d;
using vec3r = graphics::vec3r;

using Gravity = vec3r;

struct Mass : scalar<Mass, Real> {
    using scalar<Mass, Real>::scalar;
};

struct InverseMass : scalar<InverseMass, Real> {
    using scalar<InverseMass, Real>::scalar;
};

struct ParticleState {
    enum Flag {
        None = 0,
        Hovered = 1 << 0,
        Selected = 1 << 1,
        Disabled = 1 << 2,
        Pinned = 1 << 3,
        Dragged = 1 << 4,
    };

    uint32_t flags = None;

    static void meta(flecs::world& ecs) {
        ecs.component<ParticleState>()
            .bit("Hovered", (uint32_t)ParticleState::Hovered)
            .bit("Selected", (uint32_t)ParticleState::Selected)
            .bit("Disabled", (uint32_t)ParticleState::Disabled)
            .bit("Pinned", (uint32_t)ParticleState::Pinned)
            .bit("Dragged", (uint32_t)ParticleState::Dragged);
    }
};

struct IsPinned {};
struct Particle {};

namespace components {

inline void register_particle_components(flecs::world& ecs) {
    graphics::register_vec3_component<vec3f>(ecs, "vec3f");
    graphics::register_vec3_component<vec3d>(ecs, "vec3d");
    graphics::register_vec3_component<vec3r>(ecs, "vec3r");
    graphics::register_vec3_component<Position>(ecs);
    graphics::register_vec3_component<Velocity>(ecs);
    graphics::register_vec3_component<Acceleration>(ecs);
    graphics::register_vec3_component<Force>(ecs);
    graphics::register_vec3_component<OldPosition>(ecs);

    const flecs::entity_t real_meta = std::is_same_v<Real, double> ? flecs::F64 : flecs::F32;
    graphics::register_scalar_component<Mass>(ecs, real_meta);
    graphics::register_scalar_component<InverseMass>(ecs, real_meta);

    ecs.component<Particle>();
    ecs.component<IsPinned>();

    ParticleState::meta(ecs);
}

} // namespace components
