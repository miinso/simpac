#pragma once

#include "../components.h"
#include <Eigen/Sparse>
#include <cmath>

namespace systems {

constexpr double kEpsilon = 1e-6;

// =========================================================================
// Physics Kernels (integration-agnostic)
// =========================================================================

inline void collect_momentum(const Mass& mass, const Velocity& vel,
                                   const ParticleIndex& idx, Solver& solver) {
    // RHS contribution: M * v_n (momentum)
    auto momentum = mass.value * vel.value;
    solver.b.segment<3>(idx.value * 3) += momentum;
}

inline void collect_external_force(const Mass& mass, const ParticleIndex& idx,
                                    const Vector3r& gravity, Real dt, Solver& solver) {
    // RHS contribution: time-discretized external force term (h * f_ext)
    auto f_gravity = mass.value * gravity;
    solver.b.segment<3>(idx.value * 3) += dt * f_gravity;
}

inline void collect_spring_gradient(Spring& spring, Real dt, Solver& solver) {
    auto a = spring.particle_a;
    auto b = spring.particle_b;

    bool a_pinned = a.has<IsPinned>();
    bool b_pinned = b.has<IsPinned>();

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

    // RHS contribution: h * -grad(E) (elastic forces)
    // Only add contributions for non-pinned particles
    // TODO: or i could drop pinned particles from the system dofs
    if (!a_pinned) solver.b.segment<3>(idx_a * 3) += -dt * grad_a;
    if (!b_pinned) solver.b.segment<3>(idx_b * 3) += -dt * grad_b;
}

inline void update_velocity(Velocity& v, const ParticleIndex& idx, const Solver& solver) {
    v.value = solver.x.segment<3>(idx.value * 3);
    // TODO: instead accesing solver directly, (which incur atomic access),
    // (which is a nono for threading), (even tho is not a major bottleneck),
    // we could maintain per-entity x, b vector segments,
    // and do gather op later.. above is a read op so we're good, 
    // but maybe for wirte ops like "collect_" systems..
}

inline void update_position(Position& x, const Velocity& v, Real dt) {
    x.value += dt * v.value;
}

inline void ground_collision(Position& x, const OldPosition& x_old, Real dt, Real friction = 0.8) {
    if (x.value.y() < 0) {
        x.value.y() = 0;
        auto displacement = x.value - x_old.value;
        auto friction_factor = std::min(Real(1), dt * friction);
        x.value.x() = x_old.value.x() + displacement.x() * (1 - friction_factor);
        x.value.z() = x_old.value.z() + displacement.z() * (1 - friction_factor);
    }
}

// =========================================================================
// Implicit Integration Specifics (mass matrix, hessian, solver)
// =========================================================================

inline void collect_mass(const Mass& m, const ParticleIndex& idx, Solver& solver) {
    for (int i = 0; i < 3; ++i) {
        solver.triplets.push_back({idx.value*3+i, idx.value*3+i, m.value});
    }
}

inline void collect_spring_hessian(Spring& spring, Real dt, Solver& solver) {
    // maybe i should merge grad/hess evals into one system
    auto a = spring.particle_a;
    auto b = spring.particle_b;

    bool a_pinned = a.has<IsPinned>();
    bool b_pinned = b.has<IsPinned>();

    auto x_a = a.get<Position>().value;
    auto x_b = b.get<Position>().value;
    auto idx_a = a.get<ParticleIndex>().value;
    auto idx_b = b.get<ParticleIndex>().value;

    auto diff = x_a - x_b;
    auto l = diff.norm();
    if (l < kEpsilon) return;

    auto dir = diff / l;

    Real k = spring.k_s;
    Real L0 = spring.rest_length;
    Real ratio = L0 / l;

    Matrix3r I = Matrix3r::Identity();
    Matrix3r ddT = dir * dir.transpose();
    Matrix3r H_block = k * ((1.0 - ratio) * I + ratio * ddT);

    Real h2 = dt * dt;

    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            Real val = h2 * H_block(r, c);
            // diagonal blocks
            if (!a_pinned) solver.triplets.push_back({idx_a*3+r, idx_a*3+c, val});
            if (!b_pinned) solver.triplets.push_back({idx_b*3+r, idx_b*3+c, val});
            // off-diagonal blocks (only if both particles are not pinned)
            if (!a_pinned && !b_pinned) {
                solver.triplets.push_back({idx_a*3+r, idx_b*3+c, -val});
                solver.triplets.push_back({idx_b*3+r, idx_a*3+c, -val});
            }
        }
    }
}

inline void solve(Solver& solver) {
    // build sparse mat
    solver.A.setFromTriplets(solver.triplets.begin(), solver.triplets.end());

    solver.cg.setMaxIterations(solver.cg_max_iter);
    solver.cg.setTolerance(solver.cg_tolerance);

    if (solver.cg.info() == Eigen::InvalidInput) {
        solver.cg.compute(solver.A);
    } else {
        solver.cg.factorize(solver.A);
    }

    solver.x = solver.cg.solveWithGuess(solver.b, solver.x_prev); // warm start, should check same size
    // solver.x = solver.cg.solve(solver.b); // cold start
    solver.x_prev = solver.x;

    // store stats for debug display
    solver.cg_iterations = (int)solver.cg.iterations();
    solver.cg_error = solver.cg.error();

    static int frame = 0;
    if (++frame % 50 == 0 || solver.cg.info() != Eigen::Success) {
        char buf[128];
        snprintf(buf, sizeof(buf), "[%d] CG: %d iter, err=%e%s",
                 frame, solver.cg_iterations, solver.cg_error,
                 solver.cg.info() != Eigen::Success ? " FAILED" : "");

        printf("%s\n", buf);

        solver.cg_history.push_back(buf);
        if (solver.cg_history.size() > (size_t)solver.cg_history_max_lines) {
            solver.cg_history.pop_front();
        }
    }
}

} // namespace systems
