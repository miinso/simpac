#pragma once

#include <Eigen/Dense>
#include <Eigen/Sparse>
#include <flecs.h>

using Real = double;
using Vector3r = Eigen::Matrix<Real, 3, 1, Eigen::DontAlign>;
using VectorXr = Eigen::Matrix<Real, Eigen::Dynamic, 1>;
using ArrayXr = Eigen::Array<Real, Eigen::Dynamic, 1>;
using MatrixXr = Eigen::Matrix<Real, Eigen::Dynamic, Eigen::Dynamic>;

// Particle components
struct ParticleIndex { int value; };
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

struct Spring {
    flecs::entity particle_a;
    flecs::entity particle_b;
    Real rest_length;
    Real k_s;
    Real k_d;
};

struct Solver {
    Eigen::SparseMatrix<Real> A; // system matrix (3n x 3n)
    VectorXr b; // rhs vector (3n x 1)
    VectorXr x; // solution (3n x 1)

    std::vector<Eigen::Triplet<Real>> triplets;
    int particle_count = 0;
};

// Scene
struct Scene {
    Real timestep;
    int num_substeps;
    int solve_iter;
    Vector3r gravity;
    Real elapsed = 0;
    int num_particles;
};
