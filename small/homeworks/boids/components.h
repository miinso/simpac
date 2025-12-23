#pragma once

#include "common/components.h"
#include "common/spatial_hash.h"
#include "Eigen/Dense"
#include "flecs.h"
#include <vector>

namespace boids {

// Tags
struct Boid {};
struct Predator {};

// Components
struct MaxSpeed { float value; };
struct MaxForce { float value; };
struct Size { float value; };
struct BoidColor { Eigen::Vector3f value; };

struct NearbyBoids {
    std::vector<flecs::entity> neighbors;
    std::vector<flecs::entity> predators;
    std::vector<flecs::entity> obstacles;
};

struct Obstacle {
    Eigen::Vector3f position;
    float radius;
};

struct FlockingParams {
    float separation_distance;
    float alignment_distance;
    float cohesion_distance;
    float separation_weight;
    float alignment_weight;
    float cohesion_weight;
    float obstacle_avoidance_radius;
    float obstacle_avoidance_weight;
    float predator_avoidance_radius;
    float predator_avoidance_weight;
    float prey_attraction_radius;
    float prey_attraction_weight;
};

struct Scene {
    Eigen::Vector3f gravity;
    float air_resistance;
    float simulation_time_step;
    float accumulated_time;
    double frame_time;
    double sim_time;
    Eigen::Vector3f min_bounds;
    Eigen::Vector3f max_bounds;
};

struct DebugSettings {
    bool use_spatial_hash = false;
    bool show_spatial_hash = false;
};

struct BoidSystems {
    flecs::system update_hash;
    flecs::system find_neighbors_spatial;
    flecs::system find_neighbors_brute;
};

struct BoidQueries {
    flecs::query<const ::Position, Boid> boids;
    flecs::query<const ::Position, Predator> predators;
    flecs::query<const Obstacle> obstacles;
};

struct SpatialHashComponent {
    SpatialHash hash;
    std::vector<flecs::entity> entities;
};

} // namespace boids
