#pragma once

#include "components.h"
#include "common/components.h"
#include "Eigen/Dense"
#include "flecs.h"

namespace particle_mesh {

inline flecs::entity create_particle(
    flecs::world& ecs,
    const Eigen::Vector3f& pos,
    const Eigen::Vector3f& vel,
    float radius = 0.3f,
    float mass = 1.0f,
    float friction = 0.3f,
    float restitution = 0.7f) {
    return ecs.entity()
        .add<Particle>()
        .set<Position>({pos})
        .set<Velocity>({vel})
        .add<particle_lifespan>()
        .set<ParticleProperties>({radius, mass, friction, restitution, false});
}

} // namespace particle_mesh
