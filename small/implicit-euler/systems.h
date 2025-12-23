#pragma once

#include "components.h"
#include "graphics.h"
#include "flecs.h"
#include <raylib.h>
#include <cmath>

namespace systems {

constexpr double kEpsilon = 1e-6;

// =========================================================================
// Simulation kernels
// =========================================================================

inline void update_velocity(Velocity& v, const ParticleIndex& idx, const Solver& solver) {
    v.value = solver.x.segment<3>(idx.value * 3);
}

inline void update_position(Position& x, const Velocity& v, Real dt) {
    x.value += dt * v.value;
}

// inline void clear_acceleration(Acceleration& a, const Vector3r& gravity) {
//     a.value = gravity;
// }

inline void collect_external_force(const Mass& mass, const Velocity& vel, 
                                   const ParticleIndex& idx, const Vector3r& gravity,
                                   Real dt, Solver& solver) {
    // RHS contribution: M*v_n + h*f_ext
    // where f_ext = m*g (gravity)
    Vector3r f_gravity = mass.value * gravity;
    solver.b.segment<3>(idx.value * 3) += mass.value * vel.value + dt * f_gravity;
}

inline void collect_spring_gradient(Spring& spring, Real dt, Solver& solver) {
    auto a = spring.particle_a;
    auto b = spring.particle_b;

    auto x_a = a.get<Position>().value;
    auto x_b = b.get<Position>().value;
    auto v_a = a.get<Velocity>().value;
    auto v_b = b.get<Velocity>().value;
    auto idx_a = a.get<ParticleIndex>().value;
    auto idx_b = b.get<ParticleIndex>().value;

    auto diff = x_a - x_b; // we follow stuyck2018cloth. x_ij = x_i - x_j
    auto l = diff.norm();
    if (l < kEpsilon) return;
    auto dir = diff / l;
    
    auto strain = (l - spring.rest_length) / spring.rest_length; // make it unitless

    auto diff_v = v_a - v_b;
    auto relative_velocity = (diff_v / spring.rest_length).dot(dir);

    Vector3r grad_a = (spring.k_s * strain + spring.k_d * relative_velocity) * dir; // grad_a = -force_a
    Vector3r grad_b = -grad_a;

    // RHS contribution: -h*∇E (elastic forces)
    solver.b.segment<3>(idx_a * 3) += -dt * grad_a;
    solver.b.segment<3>(idx_b * 3) += -dt * grad_b;
}

inline void collect_mass(const Mass& m, const ParticleIndex& idx, Solver& solver) {
    for (int i = 0; i < 3; ++i) {
        solver.triplets.push_back({idx.value*3+i, idx.value*3+i, m.value});
    }
}

inline void collect_spring_hessian(Spring& spring, Real dt, Solver& solver) {
    auto a = spring.particle_a;
    auto b = spring.particle_b;

    auto x_a = a.get<Position>().value;
    auto x_b = b.get<Position>().value;
    auto idx_a = a.get<ParticleIndex>().value;
    auto idx_b = b.get<ParticleIndex>().value;

    auto diff = x_a - x_b;
    auto l = diff.norm();
    if (l < kEpsilon) return;
    
    auto dir = diff / l;
}

inline void solve(Solver& solver) {
    // build sparse mat
    solver.A.setFromTriplets(solver.triplets.begin(), solver.triplets.end());

    Eigen::ConjugateGradient<Eigen::SparseMatrix<Real>> cg;
    cg.setMaxIterations(solver.cg_max_iter);
    cg.setTolerance(solver.cg_tolerance);
    cg.compute(solver.A);
    solver.x = cg.solveWithGuess(solver.b, solver.x); // warm start

    // store stats for debug display
    solver.cg_iterations = (int)cg.iterations();
    solver.cg_error = cg.error();

    static int frame = 0;
    if (++frame % 60 == 0 || cg.info() != Eigen::Success) {
        printf("[%d] CG: %d iter, err=%e%s\n", frame, solver.cg_iterations, solver.cg_error,
               cg.info() != Eigen::Success ? " FAILED" : "");
    }
}


inline void ground_collision(Position& x, const OldPosition& x_old, Real dt, Real friction = 0.8) {
    if (x.value.y() < 0) {
        x.value.y() = 0;
        auto displacement = x.value - x_old.value;
        auto friction_factor = std::min(1.0, dt * friction);
        x.value.x() = x_old.value.x() + displacement.x() * (1 - friction_factor);
        x.value.z() = x_old.value.z() + displacement.z() * (1 - friction_factor);
    }
}


// =========================================================================
// Drawing
// =========================================================================

inline Vector3 toRay3(const Vector3r& v) {
    return {(float)v.x(), (float)v.y(), (float)v.z()};
}

inline void draw_spring(Spring& spring) {
    auto& x1 = spring.particle_a.get<Position>().value;
    auto& x2 = spring.particle_b.get<Position>().value;
    
    auto diff = x1 - x2;
    auto current_length = diff.norm();
    auto strain = (current_length - spring.rest_length) / spring.rest_length;
    
    // Color based on strain: compression(blue) -> rest(green) -> tension(red)
    Color color;
    if (strain > 0) {
        float t = std::min(1.0f, (float)strain * 10.0f);
        color = ColorLerp(GREEN, RED, t);
    } else {
        float t = std::min(1.0f, (float)(-strain) * 10.0f);
        color = ColorLerp(GREEN, BLUE, t);
    }
    
    float thickness = 0.002f;
    // DrawCylinderEx(toRay3(x1), toRay3(x2), thickness, thickness, 3, color);
    DrawLine3D(toRay3(x1), toRay3(x2), color);
}

inline void draw_particle(const Position& x, const Mass& m) {
    // DrawSphere(toRay3(x.value), 0.05f * std::pow((float)m.value, 1.0f / 3.0f), BLUE);
    DrawPoint3D(toRay3(x.value), BLUE);
}

inline void draw_timing_info(flecs::iter& it) {
    auto& scene = it.world().get_mut<Scene>();
    auto& solver = it.world().get<Solver>();
    scene.elapsed += it.delta_time();

    Font font = graphics::get_font();
    char buf[512];
    snprintf(buf, sizeof(buf),
        "Elapsed: %.2fs  dt=%.4f\n"
        "CG: %d/%d iter, tol=%.0e, err=%.2e\n"
        "\n"
        "Particles: %d  Springs: %d\n"
        "\n"
        "Strain: red(tension) green(rest) blue(compression)\n"
        "Camera: WASDQE / Arrows / MouseDrag / MouseScroll",
        scene.elapsed, scene.timestep,
        solver.cg_iterations, solver.cg_max_iter, solver.cg_tolerance, solver.cg_error,
        scene.num_particles, scene.num_springs);
    DrawTextEx(font, buf, {20, 40}, 12, 0, DARKGRAY);
}

} // namespace systems
