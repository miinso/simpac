#pragma once

#include <Eigen/Dense>
#include <Eigen/Sparse>
#include <flecs.h>
#include <vector>
#include <deque>
#include <string>
#include <cstddef>
#include <cstdint>
#include <type_traits>

using Real = float;
namespace Eigen {
using Vector3r = Matrix<Real, 3, 1, DontAlign>;
using Vector4r = Matrix<Real, 4, 1, DontAlign>;
using Matrix3r = Matrix<Real, 3, 3>;
using VectorXr = Matrix<Real, Dynamic, 1>;
using ArrayXr = Array<Real, Dynamic, 1>;
using MatrixXr = Matrix<Real, Dynamic, Dynamic>;
}  // namespace Eigen

// Particle components

/*
  crtp keeps vec3 components uniform and lightweight.
  plain fields for flecs/raylib; map() gives an eigen view when needed.
*/
template <typename Derived>
struct vec3 {
    Real x = Real(0);
    Real y = Real(0);
    Real z = Real(0);

    vec3() = default;
    vec3(Real x_, Real y_, Real z_) : x(x_), y(y_), z(z_) {}

    template <typename DerivedEigen>
    vec3(const Eigen::MatrixBase<DerivedEigen>& v)
        : x(Real(v.x())), y(Real(v.y())), z(Real(v.z())) {}

    Real* data() { return &x; }
    const Real* data() const { return &x; }

    Real& operator[](size_t i) { return data()[i]; }
    const Real& operator[](size_t i) const { return data()[i]; }

    Eigen::Map<Eigen::Vector3r> map() { return Eigen::Map<Eigen::Vector3r>(data()); }
    Eigen::Map<const Eigen::Vector3r> map() const { return Eigen::Map<const Eigen::Vector3r>(data()); }
};

template <typename Derived>
struct vec4 {
    Real x = Real(0);
    Real y = Real(0);
    Real z = Real(0);
    Real w = Real(0);

    vec4() = default;
    vec4(Real x_, Real y_, Real z_, Real w_) : x(x_), y(y_), z(z_), w(w_) {}

    template <typename DerivedEigen>
    vec4(const Eigen::MatrixBase<DerivedEigen>& v)
        : x(Real(v.x())), y(Real(v.y())), z(Real(v.z())), w(Real(v.w())) {}

    Real* data() { return &x; }
    const Real* data() const { return &x; }

    Real& operator[](size_t i) { return data()[i]; }
    const Real& operator[](size_t i) const { return data()[i]; }

    Eigen::Map<Eigen::Vector4r> map() { return Eigen::Map<Eigen::Vector4r>(data()); }
    Eigen::Map<const Eigen::Vector4r> map() const { return Eigen::Map<const Eigen::Vector4r>(data()); }
};

template <typename Derived, typename Value>
struct scalar {
    Value value = Value(0);

    scalar() = default;
    scalar(Value v) : value(v) {}

    scalar& operator=(Value v) {
        value = v;
        return *this;
    }

    operator Value&() { return value; }
    operator const Value&() const { return value; }
};

template <typename Derived>
using Vec3Component = vec3<Derived>;

template <typename Derived>
using Vec4Component = vec4<Derived>;

template <typename Derived, typename Value>
using ScalarComponent = scalar<Derived, Value>;

struct Position : vec3<Position> {
    using vec3<Position>::vec3;
};

struct Velocity : vec3<Velocity> { 
    using vec3<Velocity>::vec3;
};

struct Acceleration : vec3<Acceleration> {
    using vec3<Acceleration>::vec3;
};

struct OldPosition : vec3<OldPosition> {
    using vec3<OldPosition>::vec3;
};

struct Gravity : vec3<Gravity> {
    using vec3<Gravity>::vec3;
};

// struct Quaternion : vec4<Quaternion> {
//     using vec4<Quaternion>::vec4;
// };

struct Mass : scalar<Mass, Real> {
    using scalar<Mass, Real>::scalar;
};

struct InverseMass : scalar<InverseMass, Real> {
    using scalar<InverseMass, Real>::scalar;
};

struct ParticleIndex : scalar<ParticleIndex, int> {
    using scalar<ParticleIndex, int>::scalar;
};

struct ParticleState {
    enum Flag {
        None = 0,
        Hovered = 1 << 0,
        Selected = 1 << 1,
        Disabled = 1 << 2,
        Pinned = 1 << 3,
    };

    uint32_t flags = None;
};

struct SpringRestLength : scalar<SpringRestLength, Real> {
    using scalar<SpringRestLength, Real>::scalar;
};

struct SpringStiffness : scalar<SpringStiffness, Real> {
    using scalar<SpringStiffness, Real>::scalar;
};

struct SpringDamping : scalar<SpringDamping, Real> {
    using scalar<SpringDamping, Real>::scalar;
};

template <typename T>
inline void register_vec3(flecs::world& ecs) {
    ecs.component<T>()
        .member("x", &T::x)
        .member("y", &T::y)
        .member("z", &T::z);
}

template <typename T>
inline void register_vec4(flecs::world& ecs) {
    ecs.component<T>()
        .member("x", &T::x)
        .member("y", &T::y)
        .member("z", &T::z)
        .member("w", &T::w);
}

template <typename T>
inline void register_scalar(flecs::world& ecs, flecs::entity_t scalar_meta) {
    using Value = std::remove_cv_t<std::remove_reference_t<decltype(T::value)>>;
    auto opaque = ecs.component<T>().opaque(scalar_meta);
    if constexpr (std::is_integral_v<Value>) {
        opaque.serialize([](const flecs::serializer* s, const T* data) {
                if (!data) return 0;
                s->value(static_cast<int64_t>(data->value));
                return 0;
            })
            .assign_int([](T* dst, int64_t value) {
                if (!dst) return;
                dst->value = static_cast<Value>(value);
            });
    } else {
        opaque.serialize([](const flecs::serializer* s, const T* data) {
                if (!data) return 0;
                s->value(static_cast<double>(data->value));
                return 0;
            })
            .assign_float([](T* dst, double value) {
                if (!dst) return;
                dst->value = static_cast<Value>(value);
            });
    }
}

template <typename T>
inline void register_vec3_plain(flecs::world& ecs) {
    register_vec3<T>(ecs);
}

template <typename T>
inline void register_vec4_plain(flecs::world& ecs) {
    register_vec4<T>(ecs);
}

template <typename T>
inline void register_scalar_component(flecs::world& ecs, flecs::entity_t scalar_meta) {
    register_scalar<T>(ecs, scalar_meta);
}

inline void register_particle_state_flags(flecs::world& ecs) {
    ecs.component<ParticleState>()
        .bit("Hovered", (uint32_t)ParticleState::Hovered)
        .bit("Selected", (uint32_t)ParticleState::Selected)
        .bit("Disabled", (uint32_t)ParticleState::Disabled)
        .bit("Pinned", (uint32_t)ParticleState::Pinned);
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
    // NOTE: flecs::entity members are script-settable, but Flecs <= 4.1.4
    // fails to cast script identifiers to flecs::entity. See PR #1912:
    // https://github.com/SanderMertens/flecs/pull/1912
    flecs::entity e1;
    flecs::entity e2;
    SpringRestLength rest_length; // too much? just use Real?
    SpringStiffness k_s;
    SpringDamping k_d;

    // reviewer note: co-locating reflection keeps schema discoverable,
    // but couples this POD to Flecs; keep it small and call from one place.
    static void meta(flecs::world& ecs) {
        ecs.component<Spring>()
            .member("e1", &Spring::e1)
            .member("e2", &Spring::e2)
            .member("rest_length", &Spring::rest_length)
                .range(0.0, 10.0)
            .member("k_s", &Spring::k_s)
                .range(0.0, 200000.0)
            .member("k_d", &Spring::k_d)
                .range(0.0, 10.0);
    }
};

inline void register_sim_components(flecs::world& ecs) {
    register_vec3<Position>(ecs);
    register_vec3<Velocity>(ecs);
    register_vec3<Acceleration>(ecs);
    register_vec3<OldPosition>(ecs);
    register_vec3<Gravity>(ecs);
    // register_vec4<Quaternion>(ecs); // collides with raylib's..

    register_scalar<Mass>(ecs, flecs::F32);
    register_scalar<InverseMass>(ecs, flecs::F32);
    register_scalar<ParticleIndex>(ecs, flecs::I32);
    register_scalar<SpringRestLength>(ecs, flecs::F32);
    register_scalar<SpringStiffness>(ecs, flecs::F32);
    register_scalar<SpringDamping>(ecs, flecs::F32);

    register_particle_state_flags(ecs);

    Spring::meta(ecs);
}

struct Solver {
    // TODO: decide if we want to maintain one global solver or go per-object
    Eigen::SparseMatrix<Real> A;    // system matrix (3n x 3n)
    Eigen::VectorXr b;                     // rhs vector (3n x 1)
    Eigen::VectorXr x;                     // solution (3n x 1)
    Eigen::VectorXr x_prev;                // prev soultion (3n x 1)

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
    Real dt = Real(1.0f / 60.0f);     // timestep per simulation step
    Gravity gravity = {0.0f, -9.81f, 0.0f};  // gravity vector

    Real wall_time = 0;         // real elapsed time (wall-clock)
    Real sim_time = 0;          // accumulated simulation time
    int frame_count = 0;        // number of simulation steps executed

    // cached queries (initialized via on_set hook)
    flecs::query<Position> particle_query;
    flecs::query<Spring> spring_query;

    // query-based count accessors
    int num_particles() const { return (int)particle_query.count(); }
    int num_springs() const { return (int)spring_query.count(); }

    // control flags
    bool paused = false;        // simulation pause state
    Real sim_speed = 1.0;       // simulation speed multiplier

    static void meta(flecs::world& ecs) {
        ecs.component<Scene>()
            .member("dt", &Scene::dt)
            .member("gravity", &Scene::gravity)
            .member("paused", &Scene::paused)
            .member("sim_speed", &Scene::sim_speed);
    }
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
