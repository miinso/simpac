#pragma once

#include <Eigen/Dense>
#include <flecs.h>

using Real = float;
using Vector3r = Eigen::Matrix<Real, 3, 1, Eigen::DontAlign>;

// Particle components
struct Position { Vector3r value; };
struct Velocity { Vector3r value; };
struct Acceleration { Vector3r value; };
struct OldPosition { Vector3r value; };
struct Mass { Real value; };
struct InverseMass { Real value; };

// Tags
struct IsPinned {};
struct Particle {};
struct Constraint {};

// Constraints
struct DistanceConstraint {
    flecs::entity e1;
    flecs::entity e2;
    Real rest_distance;
    Real stiffness;
    Real lambda = 0;
};

// Scene configuration and state
struct Scene {
    // Simulation parameters
    Real dt;                    // timestep per simulation step
    int num_substeps;           // PBD substeps
    int solve_iter;             // constraint solver iterations
    Vector3r gravity;           // gravity vector
    
    // Runtime state
    Real wall_time = 0;         // real elapsed time (wall-clock)
    Real sim_time = 0;          // accumulated simulation time
    int frame_count = 0;        // number of simulation steps executed
};
