#pragma once

#include "components.h"
#include "common/components.h"
#include "Eigen/Dense"
#include "flecs.h"

namespace boids {

inline flecs::entity create_boid(
    flecs::world& ecs,
    const Eigen::Vector3f& pos,
    const Eigen::Vector3f& vel,
    float max_speed = 5.0f,
    float max_force = 2.0f,
    float size = 0.1f,
    const Eigen::Vector3f& color = Eigen::Vector3f::Random().cwiseAbs()) {
    return ecs.entity()
        .add<Boid>()
        .set<Position>({pos})
        .set<Velocity>({vel})
        .set<Acceleration>({Eigen::Vector3f::Zero()})
        .set<MaxSpeed>({max_speed})
        .set<MaxForce>({max_force})
        .set<Size>({size})
        .set<BoidColor>({color})
        .set<NearbyBoids>({});
}

inline flecs::entity create_predator(
    flecs::world& ecs,
    const Eigen::Vector3f& pos,
    const Eigen::Vector3f& vel,
    float max_speed = 1.0f,
    float max_force = 0.5f,
    float size = 0.5f,
    const Eigen::Vector3f& color = Eigen::Vector3f(0.2f, 0.2f, 0.2f)) {
    return ecs.entity()
        .add<Predator>()
        .set<Position>({pos})
        .set<Velocity>({vel})
        .set<Acceleration>({Eigen::Vector3f::Zero()})
        .set<MaxSpeed>({max_speed})
        .set<MaxForce>({max_force})
        .set<Size>({size})
        .set<BoidColor>({color});
}

inline flecs::entity create_obstacle(
    flecs::world& ecs,
    const Eigen::Vector3f& pos,
    float radius) {
    return ecs.entity().set<Obstacle>({pos, radius});
}

} // namespace boids
