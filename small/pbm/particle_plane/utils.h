#pragma once

#include "components.h"
#include "common/components.h"
#include "Eigen/Dense"
#include "flecs.h"

namespace particle_plane {

inline flecs::entity create_particle(
    flecs::world& ecs,
    const Eigen::Vector3f& pos,
    const Eigen::Vector3f& vel,
    float radius = 0.5f,
    float mass = 1.0f,
    float friction = 0.3f,
    float restitution = 0.7f) {
    return ecs.entity()
        .add<Particle>()
        .set<Position>({pos})
        .set<Velocity>({vel})
        .set<ParticleProperties>({radius, mass, friction, restitution, false});
}

inline flecs::entity create_plane(
    flecs::world& ecs,
    const Eigen::Vector3f& pos,
    const Eigen::Vector3f& normal,
    float friction = 0.3f,
    float restitution = 0.7f,
    bool visible = true,
    bool audible = true) {
    return ecs.entity()
        .add<Plane>()
        .set<Position>({pos})
        .set<PlaneProperties>({normal, friction, restitution, 0, visible, audible});
}

} // namespace particle_plane
