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

// Scene
struct Scene {
    Real timestep;
    int num_substeps;
    int solve_iter;
    Vector3r gravity;
    Real elapsed = 0;
};
