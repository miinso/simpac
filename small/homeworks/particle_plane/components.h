#pragma once

#include "common/components.h"
#include "Eigen/Dense"
#include "flecs.h"
#include "raylib.h"

namespace particle_plane {

// Constants
constexpr float COLLISION_EPSILON = 1e-6f;
constexpr float COLLISION_THRESHOLD = 0.01f;
constexpr float RESTING_VELOCITY_THRESHOLD = 0.01f;
constexpr float RESTING_DISTANCE_THRESHOLD = 0.01f;
constexpr float RESTING_FORCE_THRESHOLD = 0.01f;

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

// Audio state
struct AudioState {
    Sound notes[5];
    Sound* sequence[9];
    int collision_sfx_count = 0;
};

} // namespace particle_plane
