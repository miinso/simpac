#pragma once

#include <Eigen/Dense>
#include <Eigen/Sparse>
#include <flecs.h>
#include <vector>
#include <deque>
#include <string>

using Real = float;
using Vector3r = Eigen::Matrix<Real, 3, 1, Eigen::DontAlign>;
using Matrix3r = Eigen::Matrix<Real, 3, 3>;
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
    VectorXr x_prev; // prev soultion (3n x 1)

    std::vector<Eigen::Triplet<Real>> triplets;
    int particle_count = 0;

    // CG solver config
    int cg_max_iter = 100;
    Real cg_tolerance = 1e-6;

    // CG solver stats (updated each solve)
    int cg_iterations = 0;
    Real cg_error = 0;

    // CG history log (for display)
    std::deque<std::string> cg_history;
    int cg_history_max_lines = 15;

    // Eigen::ConjugateGradient<Eigen::SparseMatrix<Real>, Eigen::Lower|Eigen::Upper, Eigen::IncompleteCholesky<Real>> cg;
    Eigen::ConjugateGradient<Eigen::SparseMatrix<Real>, Eigen::Lower|Eigen::Upper, Eigen::IdentityPreconditioner> cg;
    // Eigen::ConjugateGradient<Eigen::SparseMatrix<Real>> cg;
};

// Scene configuration and state
struct Scene {
    // Simulation parameters
    Real dt;                    // timestep per simulation step
    Vector3r gravity;           // gravity vector
    
    // Runtime state
    Real wall_time = 0;         // real elapsed time (wall-clock)
    Real sim_time = 0;          // accumulated simulation time
    int frame_count = 0;        // number of simulation steps executed
    
    // Scene statistics (updated by systems)
    int num_particles = 0;
    int num_springs = 0;
    
    // Optional: control flags
    bool paused = false;        // simulation pause state
    Real sim_speed = 1.0;       // simulation speed multiplier
};

// GPU Spring Renderer
struct SpringRenderer {
    // resources
    unsigned int vao = 0;
    unsigned int vbo = 0;
    unsigned int shader_id = 0;

    // uniform locations
    int u_viewproj_loc = -1;
    int u_strain_scale_loc = -1;

    // some metadata
    int num_springs = 0;
    int num_particles = 0;

    // CPU-side staging buffer (updated each frame)
    // Layout: [pos_a.x, pos_a.y, pos_a.z, pos_b.x, pos_b.y, pos_b.z, rest_len, endpoint] per vertex
    // 8 floats per vertex, 2 vertices per spring
    std::vector<float> staging_buffer;

    // static spring data (built once during init)
    std::vector<int> spring_particle_indices;  // [idx_a, idx_b, ...] for each spring
    std::vector<float> rest_lengths;           // Rest length for each spring
};
