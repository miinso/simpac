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
    // Eigen::ConjugateGradient<Eigen::SparseMatrix<Real>, Eigen::Lower|Eigen::Upper, Eigen::IdentityPreconditioner> cg;
    Eigen::ConjugateGradient<Eigen::SparseMatrix<Real>> cg;
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

    // Cached queries (initialized via on_set hook)
    flecs::query<ParticleIndex> particle_query;
    flecs::query<Spring> spring_query;

    // Query-based count accessors
    int num_particles() const { return (int)particle_query.count(); }
    int num_springs() const { return (int)spring_query.count(); }

    // Control flags
    bool paused = false;        // simulation pause state
    Real sim_speed = 1.0;       // simulation speed multiplier
};

// Pin modes for cloth
enum class PinMode : int {
    Corners = 0,    // pin top-left and top-right corners
    TopRow = 1,     // pin entire top row
    None = 2        // no pinning
};

// Grid-based cloth component
// When set on an entity, creates particles and springs as children
struct GridCloth {
    // Geometry
    int width = 10;             // particles in x direction
    int height = 10;            // particles in z direction
    float spacing = 1.0f;       // distance between adjacent particles

    // Physics parameters
    float mass = 1.0f;          // mass per particle
    float k_structural = 10000.0f;  // structural spring stiffness
    float k_shear = 10000.0f;       // shear (diagonal) spring stiffness
    float k_bending = 100.0f;       // bending spring stiffness
    float k_damping = 0.0f;         // velocity damping coefficient

    // Position offset
    float offset[3] = {0, 0, 0};

    // Pinning
    int pin_mode = 0;           // 0=corners, 1=top_row, 2=none

    // Runtime info (read-only, populated by hook)
    int particle_count = 0;
    int spring_count = 0;
};

// gpu spring renderer (instanced)
struct SpringRenderer {
    // instance layout constants
    static constexpr int FLOATS_PER_INSTANCE = 7;  // pos_a(3) + pos_b(3) + rest_len(1)
    static constexpr int VERTICES_PER_INSTANCE = 2; // 2 verts drawn per spring instance

    // gpu resources
    unsigned int vao = 0;
    unsigned int instance_vbo = 0;  // per-instance attributes
    unsigned int shader_id = 0;

    // uniform locations
    int u_viewproj_loc = -1;
    int u_strain_scale_loc = -1;

    // rendering params
    float strain_scale = 10.0f;

    // current buffer allocation (for lazy resize)
    int allocated_springs = 0;

    // cached query (initialized via on_set hook)
    flecs::query<const Position, const ParticleIndex> position_query;

    // cpu-side staging buffer (updated each frame)
    // layout: [pos_a.xyz, pos_b.xyz, rest_len] per instance = 7 floats per spring
    std::vector<float> staging_buffer;

    // spring connectivity (rebuilt on topology change)
    std::vector<int> spring_particle_indices;  // [idx_a, idx_b, ...] for each spring
    std::vector<float> rest_lengths;           // Rest length for each spring
};
