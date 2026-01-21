#pragma once

#include <Eigen/Dense>
#include <Eigen/Sparse>
#include <flecs.h>
#include <vector>
#include <deque>
#include <string>
#include <cstdint>
#include <cstring>

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
struct ParticleState {
    enum Flag : uint8_t {
        None = 0,
        Hovered = 1 << 0,
        Selected = 1 << 1,
        Disabled = 1 << 2,
        Pinned = 1 << 3,
    };
    uint8_t flags = None;
};

// =========================================================================
// Reflection helpers
// =========================================================================

inline void register_vector3_reflection(flecs::world& ecs) {
    auto vec3f_meta = ecs.component()
        .member<float>("x")
        .member<float>("y")
        .member<float>("z");

    ecs.component<Vector3r>()
        .opaque(vec3f_meta)
        .serialize([](const flecs::serializer* s, const Vector3r* data) {
            if (!data) return 0;
            const float* v = data->data();
            s->member("x");
            s->value(v[0]);
            s->member("y");
            s->value(v[1]);
            s->member("z");
            s->value(v[2]);
            return 0;
        })
        .ensure_member([](Vector3r* dst, const char* member) -> void* {
            if (!dst || !member) return nullptr;
            float* v = dst->data();
            if (!std::strcmp(member, "x")) return &v[0];
            if (!std::strcmp(member, "y")) return &v[1];
            if (!std::strcmp(member, "z")) return &v[2];
            return nullptr;
        });
}

inline void register_sim_components(flecs::world& ecs) {
    register_vector3_reflection(ecs);

    ecs.component<Position>().member<Vector3r>("value");
    ecs.component<Velocity>().member<Vector3r>("value");
    ecs.component<Acceleration>().member<Vector3r>("value");
    ecs.component<OldPosition>().member<Vector3r>("value");
}

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
    // TODO: decide if we want to maintain one global solver or go per-object
    Eigen::SparseMatrix<Real> A;    // system matrix (3n x 3n)
    VectorXr b;                     // rhs vector (3n x 1)
    VectorXr x;                     // solution (3n x 1)
    VectorXr x_prev;                // prev soultion (3n x 1)

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

// scene configuration and state
struct Scene {
    Real dt;                    // timestep per simulation step
    Vector3r gravity;           // gravity vector

    Real wall_time = 0;         // real elapsed time (wall-clock)
    Real sim_time = 0;          // accumulated simulation time
    int frame_count = 0;        // number of simulation steps executed

    // cached queries (initialized via on_set hook)
    flecs::query<ParticleIndex> particle_query;
    flecs::query<Spring> spring_query;

    // query-based count accessors
    int num_particles() const { return (int)particle_query.count(); }
    int num_springs() const { return (int)spring_query.count(); }

    // control flags
    bool paused = false;        // simulation pause state
    Real sim_speed = 1.0;       // simulation speed multiplier
};

struct InteractionState {
    flecs::entity hovered;
    flecs::entity selected;
    float pick_radius_scale = 1.2f;
};

// Pin modes for cloth
// TODO: remove pinmode
enum class PinMode : int {
    Corners = 0,    // pin top-left and top-right corners
    TopRow = 1,     // pin entire top row
    None = 2        // no pinning
};

struct GridCloth {
    int width = 10;             // particles in x direction
    int height = 10;            // particles in z direction
    float spacing = 1.0f;       // distance between adjacent particles

    float mass = 1.0f;          // mass per particle
    float k_structural = 10000.0f;  // structural spring stiffness
    float k_shear = 10000.0f;       // shear (diagonal) spring stiffness
    float k_bending = 100.0f;       // bending spring stiffness
    float k_damping = 0.0f;         // velocity damping coefficient

    // position offset
    float offset[3] = {0, 0, 0};

    // pinning
    // TODO: verbose. to be removed
    int pin_mode = 0;           // 0=corners, 1=top_row, 2=none

    // runtime info (read-only, populated by hook)
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
    std::vector<float> rest_lengths;           // rest length for each spring
};

// gpu particle renderer (instanced icospheres)
struct ParticleRenderer {
    // instance layout constants
    static constexpr int FLOATS_PER_INSTANCE = 5;  // pos(3) + radius(1) + state(1)

    // gpu resources
    unsigned int vao = 0;
    unsigned int mesh_vbo = 0;       // icosphere vertices (static)
    unsigned int mesh_ebo = 0;       // icosphere indices (static)
    unsigned int instance_vbo = 0;   // per-particle data (dynamic)
    unsigned int shader_id = 0;

    // mesh data
    int num_vertices = 0;
    int num_indices = 0;

    // uniform locations
    int u_viewproj_loc = -1;
    int u_color_loc = -1;

    // rendering params
    float base_radius = 0.5f;  // base sphere size
    float color[3] = {0.2f, 0.5f, 0.9f};  // particle color

    // current buffer allocation
    int allocated_particles = 0;

    // cached query (initialized via on_set hook)
    flecs::query<const Position, const ParticleIndex, const ParticleState> position_query;

    // cpu-side staging buffer (updated each frame)
    // layout: [pos.xyz, radius, state] per instance = 5 floats per particle
    std::vector<float> staging_buffer;
};
