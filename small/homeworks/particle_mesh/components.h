#pragma once

#include "common/components.h"
#include "common/spatial_hash.h"
#include "Eigen/Dense"
#include "flecs.h"
#include "raylib.h"
#include <vector>

namespace particle_mesh {

// Tags
struct Particle {};
struct Plane {};

// Components
struct ParticleProperties {
    float radius;
    float mass;
    float friction;
    float restitution;
    bool is_rest;
    float invmass;
    float size_decay;
    float color_decay;
    float velocity_decay;
    float lifespan = 3;
};

struct particle_lifespan {
    particle_lifespan() { t = 0; }
    float t;
};

struct PlaneProperties {
    Eigen::Vector3f normal;
    float friction;
    float restitution;
    int collision_count;
    bool is_visible;
    bool is_audible;
};

struct Scene {
    Eigen::Vector3f gravity;
    float air_resistance;
    float simulation_time_step;
    float accumulated_time;
    double frame_time;
    double sim_time;
    float min_frequency = 30;
    float max_frequency = 10000;
    float frequency_slider;
};

struct SimulationResources {
    Model sphere_model;
    Model plane_model;
    Model cube_model;
    Texture2D texture;
};

struct DiskGenerator {
    Eigen::Vector3f center;
    Eigen::Vector3f normal;
    float radius;
    float particleRate;
    float accumulatedParticles;
};

// Spatial hash component for ECS storage
struct SpatialHashComponent {
    TriangleSpatialHash hash;
};

} // namespace particle_mesh
