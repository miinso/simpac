#pragma once

#include "../components.h"
#include "../physics.h"
#include "../queries.h"
#include "../vars.h"

#include <cstdio>
#include <memory>

namespace systems::bench {

namespace flow {
// step functions used by the benchmark pipeline

inline void prepare_solver(flecs::iter& it, const flecs::entity& scene_dirty) {
    auto world = it.world();
    auto& solver = world.get_mut<Solver>();

    const int n = queries::num_particles();
    const int dof = n * 3;

    const bool resized = solver.b.size() != dof;
    if (resized) {
        solver.b.resize(dof);
        solver.x.resize(dof);
        solver.x_prev.resize(dof);
        solver.A.resize(dof, dof);
    }

    const bool invalid_state = (solver.x.size() > 0)
        && (!solver.x.allFinite() || !solver.x_prev.allFinite());
    if (invalid_state) {
        solver.exploded = true;
    }

    if (scene_dirty.get<bool>() || solver.exploded || resized) {
        solver.x.setZero();
        solver.x_prev.setZero();
        solver.exploded = false;
    }

    solver.b.setZero();
    solver.triplets.clear();
}

inline void collect_momentum(flecs::iter&, size_t, const Mass& m, const Velocity& v,
                             const ParticleIndex& idx, Solver& solver) {
    solver.b.segment<3>(idx * 3) += m * v.map();
}

inline void collect_external_force(flecs::iter& it, size_t, const Mass& m,
                                   const ParticleIndex& idx, Solver& solver,
                                   const vec3f& gravity) {
    solver.b.segment<3>(idx * 3) += it.delta_time() * (m * gravity.map());
}

inline void collect_spring_gradient(flecs::iter& it, size_t, Spring& spring, Solver& solver) {
    physics::spring::Sample sample;
    if (!physics::spring::sample(spring, sample, true)) return;

    const Eigen::Vector3r grad_a = physics::spring::grad(spring.k_s, spring.k_d, sample.eval);
    const Eigen::Vector3r grad_b = -grad_a;

    if (!sample.a_pinned) solver.b.segment<3>(sample.idx_a * 3) += -it.delta_time() * grad_a;
    if (!sample.b_pinned) solver.b.segment<3>(sample.idx_b * 3) += -it.delta_time() * grad_b;
}

inline void collect_mass(flecs::iter&, size_t, const Mass& m, const ParticleIndex& idx, Solver& solver) {
    for (int i = 0; i < 3; ++i) {
        solver.triplets.push_back({idx * 3 + i, idx * 3 + i, m});
    }
}

inline void collect_spring_hessian(flecs::iter& it, size_t, Spring& spring, Solver& solver) {
    physics::spring::Sample sample;
    if (!physics::spring::sample(spring, sample, true)) return;

    const Eigen::Matrix3r h_block = physics::spring::hess(spring.k_s, spring.rest_length, sample.eval);
    const Real h2 = it.delta_time() * it.delta_time();

    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            const Real val = h2 * h_block(r, c);
            if (!sample.a_pinned) solver.triplets.push_back({sample.idx_a * 3 + r, sample.idx_a * 3 + c, val});
            if (!sample.b_pinned) solver.triplets.push_back({sample.idx_b * 3 + r, sample.idx_b * 3 + c, val});
            if (!sample.a_pinned && !sample.b_pinned) {
                solver.triplets.push_back({sample.idx_a * 3 + r, sample.idx_b * 3 + c, -val});
                solver.triplets.push_back({sample.idx_b * 3 + r, sample.idx_a * 3 + c, -val});
            }
        }
    }
}

inline void solve(flecs::iter& it, int cg_max_iter, Real cg_tolerance) {
    auto& solver = it.world().get_mut<Solver>();

    solver.A.setFromTriplets(solver.triplets.begin(), solver.triplets.end());
    solver.cg.setMaxIterations(cg_max_iter);
    solver.cg.setTolerance(cg_tolerance);

    if (solver.cg.info() == Eigen::InvalidInput) {
        solver.cg.compute(solver.A);
    } else {
        solver.cg.factorize(solver.A);
    }

    solver.x = solver.cg.solveWithGuess(solver.b, solver.x_prev);
    solver.x_prev = solver.x;

    solver.cg_iterations = (int)solver.cg.iterations();
    solver.cg_error = solver.cg.error();
    const bool cg_success = solver.cg.info() == Eigen::Success;
    if (!cg_success || !solver.x.allFinite() || !solver.x_prev.allFinite()) {
        solver.exploded = true;
    }

    char buf[128];
    std::snprintf(buf, sizeof(buf), "CG: %d iter, err=%e%s",
        solver.cg_iterations,
        solver.cg_error,
        !cg_success ? " FAILED" : "");

    if (!cg_success) {
        std::printf("%s\n", buf);
    }

    const int frame_count = state::frame_count.get<int>();
    if (!cg_success || (frame_count % 50 == 0)) {
        solver.cg_history.push_back(buf);
        if (solver.cg_history.size() > (size_t)solver.cg_history_max_lines) {
            solver.cg_history.pop_front();
        }
    }
}

inline void update_velocity(flecs::iter&, size_t, Velocity& v, const ParticleIndex& idx, const Solver& solver) {
    v.map() = solver.x.segment<3>(idx * 3);
}

inline void integrate_position(flecs::iter& it, size_t, Position& x, const Velocity& v) {
    x.map() += it.delta_time() * v.map();
}

} // namespace flow

struct handle {
    flecs::system algorithm;
};

struct seq {
    flecs::system prepare_solver;
    flecs::system collect_momentum;
    flecs::system collect_external_force;
    flecs::system collect_spring_gradient;
    flecs::system collect_mass;
    flecs::system collect_spring_hessian;
    flecs::system solve;
    flecs::system update_velocity;
    flecs::system integrate_position;
};

inline handle install(flecs::world& ecs,
                      flecs::entity cg_max_iter,
                      flecs::entity cg_tolerance) {
    // shared lifetime across flecs callbacks that outlive this function
    auto s = std::make_shared<seq>();

    handle out;
    // driver system executes the full sequence in fixed order
    out.algorithm = ecs.system("Implicit Euler")
        .kind(flecs::PreUpdate)
        .run([s](flecs::iter&) {
            const Real dt = props::dt.get<Real>();

            s->prepare_solver.run(dt);
            s->collect_momentum.run(dt);
            s->collect_external_force.run(dt);
            s->collect_spring_gradient.run(dt);
            s->collect_mass.run(dt);
            s->collect_spring_hessian.run(dt);
            s->solve.run();
            s->update_velocity.run(dt);
            s->integrate_position.run(dt);
        });

    ecs.scope(out.algorithm, [&] {
        s->prepare_solver = ecs.system("Prepare Solver")
            .kind(0)
            .run([](flecs::iter& it) {
                flow::prepare_solver(it, state::dirty);
            });

        s->collect_momentum = ecs.system<const Mass, const Velocity, const ParticleIndex, Solver>("Collect Momentum")
            .with<Particle>()
            .without<IsPinned>()
            .kind(0)
            .each(flow::collect_momentum);

        s->collect_external_force = ecs.system<const Mass, const ParticleIndex, Solver>("Collect External Force")
            .with<Particle>()
            .without<IsPinned>()
            .kind(0)
            .each([](flecs::iter& it, size_t i, const Mass& m, const ParticleIndex& idx, Solver& solver) {
                flow::collect_external_force(it, i, m, idx, solver, props::gravity.get<vec3f>());
            });

        s->collect_spring_gradient = ecs.system<Spring, Solver>("Collect Spring Gradient")
            .with<Spring>()
            .kind(0)
            .each(flow::collect_spring_gradient);

        s->collect_mass = ecs.system<const Mass, const ParticleIndex, Solver>("Collect Mass")
            .with<Particle>()
            .without<IsPinned>()
            .kind(0)
            .each(flow::collect_mass);

        s->collect_spring_hessian = ecs.system<Spring, Solver>("Collect Spring Hessian")
            .with<Spring>()
            .kind(0)
            .each(flow::collect_spring_hessian);

        s->solve = ecs.system("Solve Linear System")
            .kind(0)
            .run([cg_max_iter, cg_tolerance](flecs::iter& it) {
                flow::solve(it, cg_max_iter.get<int>(), cg_tolerance.get<Real>());
            });

        s->update_velocity = ecs.system<Velocity, const ParticleIndex, const Solver>("Update Velocity")
            .with<Particle>()
            .without<IsPinned>()
            .kind(0)
            .each(flow::update_velocity);

        s->integrate_position = ecs.system<Position, const Velocity>("Integrate Position")
            .with<Particle>()
            .without<IsPinned>()
            .kind(0)
            .each(flow::integrate_position);
    });

    return out;
}

} // namespace systems::bench
