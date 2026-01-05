#pragma once

#define EIGEN_DONT_VECTORIZE
#include <Eigen/Dense>

#include "graphics.h"

using Vector3f = Eigen::Matrix<float, 3, 1, Eigen::DontAlign>;

struct Position { Eigen::Vector3f value; };
struct Velocity { Eigen::Vector3f value; };
struct Acceleration { Eigen::Vector3f value; };
struct OldPosition { Eigen::Vector3f value; };
struct Mass { float value; };
struct InverseMass { float value; };

struct IsPinned { };

struct Particle {};

// Scene configuration and state
struct Scene {
    // Simulation parameters
    float dt;                   // timestep per simulation step
    int num_substeps;           // PBD substeps
    int solve_iter;             // constraint solver iterations
    Vector3f gravity;           // gravity vector
    
    // Runtime state
    float wall_time = 0;        // real elapsed time (wall-clock)
    float sim_time = 0;         // accumulated simulation time
    int frame_count = 0;        // number of simulation steps executed
};