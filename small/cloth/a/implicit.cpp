// Mass-Spring Simulator (implicit euler, per-entity)

#include "../setup.h"
#include "../math/spring.h"
#include <Eigen/Sparse>

// =========================================================================
// props
// =========================================================================

namespace props {
inline flecs::entity cg_max_iter;
inline flecs::entity cg_tolerance;
} // namespace props

// cg solver (file-private)
namespace cg {
inline Eigen::SparseMatrix<Real> A;
inline Eigen::VectorXr b, x, x_prev;
inline std::vector<Eigen::Triplet<Real>> triplets;
inline Eigen::ConjugateGradient<Eigen::SparseMatrix<Real>> solver;
inline bool exploded = false;
inline int iterations = 0;
inline Real error = 0;
} // namespace cg

// =========================================================================
// flow -- per-entity systems use .each() with entity_to_index for buffer mapping
// =========================================================================

namespace flow {

// -- prepare --

inline void prepare(flecs::iter&) {
    const int dof = sim::model.particle_count * 3;
    if (cg::b.size() != dof) {
        cg::b.resize(dof); cg::x.resize(dof); cg::x_prev.resize(dof);
        cg::A.resize(dof, dof);
        cg::x.setZero(); cg::x_prev.setZero();
    }
    if (cg::x.size() > 0 && (!cg::x.allFinite() || !cg::x_prev.allFinite()))
        cg::exploded = true;
    if (cg::exploded) {
        cg::x.setZero(); cg::x_prev.setZero(); cg::exploded = false;
    }
    cg::b.setZero();
    cg::triplets.clear();
}

// -- per-entity assembly --

inline void assemble_momentum(flecs::iter& it, size_t i,
                             const Mass& m, const Velocity& v) {
    int idx = sim::bridge.entity_to_index[it.entity(i).id()];
    cg::b.segment<3>(idx * 3) += (Real)m * v.map();
}

inline void assemble_external_force(flecs::iter& it, size_t i,
                                   const Mass& m) {
    int idx = sim::bridge.entity_to_index[it.entity(i).id()];
    cg::b.segment<3>(idx * 3) += it.delta_time() * (Real)m * sim::gravity;
}

inline void assemble_inertia(flecs::iter& it, size_t i,
                         const Mass& m) {
    int idx = sim::bridge.entity_to_index[it.entity(i).id()];
    for (int d = 0; d < 3; d++)
        cg::triplets.push_back({idx * 3 + d, idx * 3 + d, (Real)m});
}

// -- buffer-direct spring assembly (cross-entity needs buffer access) --

inline void assemble_spring_force(flecs::iter& it) {
    const Real dt = it.delta_time();
    for (int s = 0; s < sim::model.spring_count; s++) {
        const int i = sim::model.spring_indices[s * 2];
        const int j = sim::model.spring_indices[s * 2 + 1];

        physics::spring::Eval e;
        if (!physics::spring::eval(sim::state_0.q(i), sim::state_0.q(j),
                                   sim::state_0.qd(i), sim::state_0.qd(j),
                                   sim::model.spring_rest_length[s], e)) continue;

        const auto g = physics::spring::grad(sim::model.spring_stiffness[s],
                                             sim::model.spring_damping[s], e);
        if (!sim::bridge.particle_entities[i].has<IsPinned>()) cg::b.segment<3>(i * 3) -= dt * g;
        if (!sim::bridge.particle_entities[j].has<IsPinned>()) cg::b.segment<3>(j * 3) += dt * g;
    }
}

inline void assemble_spring_stiffness(flecs::iter& it) {
    const Real h2 = it.delta_time() * it.delta_time();
    for (int s = 0; s < sim::model.spring_count; s++) {
        const int i = sim::model.spring_indices[s * 2];
        const int j = sim::model.spring_indices[s * 2 + 1];

        physics::spring::Eval e;
        if (!physics::spring::eval(sim::state_0.q(i), sim::state_0.q(j),
                                   sim::state_0.qd(i), sim::state_0.qd(j),
                                   sim::model.spring_rest_length[s], e)) continue;

        const Eigen::Matrix3r H = physics::spring::hess(
            sim::model.spring_stiffness[s], sim::model.spring_rest_length[s], e);
        const bool i_free = !sim::bridge.particle_entities[i].has<IsPinned>();
        const bool j_free = !sim::bridge.particle_entities[j].has<IsPinned>();

        for (int r = 0; r < 3; r++) {
            for (int c = 0; c < 3; c++) {
                const Real val = h2 * H(r, c);
                if (i_free) cg::triplets.push_back({i*3+r, i*3+c, val});
                if (j_free) cg::triplets.push_back({j*3+r, j*3+c, val});
                if (i_free && j_free) {
                    cg::triplets.push_back({i*3+r, j*3+c, -val});
                    cg::triplets.push_back({j*3+r, i*3+c, -val});
                }
            }
        }
    }
}

// -- solve --

inline void solve(flecs::iter&) {
    cg::A.setFromTriplets(cg::triplets.begin(), cg::triplets.end());
    cg::solver.setMaxIterations(props::cg_max_iter.get<int>());
    cg::solver.setTolerance(props::cg_tolerance.get<Real>());
    cg::solver.compute(cg::A);
    cg::x = cg::solver.solveWithGuess(cg::b, cg::x_prev);
    cg::x_prev = cg::x;
    cg::iterations = (int)cg::solver.iterations();
    cg::error = cg::solver.error();
    if (cg::solver.info() != Eigen::Success || !cg::x.allFinite())
        cg::exploded = true;
}

// -- per-entity write-back --

inline void update_velocity(flecs::iter& it, size_t i, Velocity& v) {
    if (cg::exploded) return;
    int idx = sim::bridge.entity_to_index[it.entity(i).id()];
    v.map() = cg::x.segment<3>(idx * 3);
}

inline void integrate_position(flecs::iter& it, size_t, Position& pos, const Velocity& v) {
    pos.map() += it.delta_time() * v.map();
}

// -- stats --

inline void stats(flecs::iter& it) {
    auto& ui = it.world().ensure<Solver>();
    ui.cg_iterations = cg::iterations;
    ui.cg_error = cg::error;
    ui.exploded = cg::exploded;

    char buf[128];
    snprintf(buf, sizeof(buf), "CG: %d iter, err=%e%s",
             cg::iterations, cg::error,
             cg::exploded ? " FAILED" : "");

    const int frame = state::frame_count.get<int>();
    if (cg::exploded || (frame % 50 == 0)) {
        ui.cg_history.push_back(buf);
        if (ui.cg_history.size() > (size_t)ui.cg_history_max_lines)
            ui.cg_history.pop_front();
    }
    if (cg::exploded) printf("%s\n", buf);
}

} // namespace flow

// =========================================================================
// main
// =========================================================================

int main() {
    printf("Hi from %s\n", __FILE__);

    flecs::world ecs;
    sim::init(ecs, {800, 600, "Implicit Euler (ecs)"});

    props::cg_max_iter = ecs.entity("Config::Solver::cg_max_iter").set<int>(100).add<Configurable>();
    props::cg_tolerance = ecs.entity("Config::Solver::cg_tolerance").set<Real>(Real(1e-3f)).add<Configurable>();

    sim::install(ecs);

    // -- implicit euler (per-entity assembly, buffer-direct springs) ----------

    flecs::system prepare, assemble_momentum, assemble_external_force, assemble_inertia;
    flecs::system assemble_spring_force, assemble_spring_stiffness;
    flecs::system solve, update_velocity, integrate_position;

    auto integrator = ecs.system("Implicit Euler")
        .kind(sim::Simulate)
        .run([&](flecs::iter&) {
            const Real dt = props::dt.get<Real>();
            sim::gravity = props::gravity.get<vec3f>().map();
            prepare.run(dt);
            assemble_momentum.run(dt);
            assemble_external_force.run(dt);
            assemble_spring_force.run(dt);
            assemble_inertia.run(dt);
            assemble_spring_stiffness.run(dt);
            solve.run(dt);
            update_velocity.run(dt);
            integrate_position.run(dt);
        });

    ecs.scope(integrator, [&] {
        prepare = ecs.system("Prepare")
            .kind(0)
            .run(flow::prepare);

        assemble_momentum = ecs.system<const Mass, const Velocity>("Assemble Momentum")
            .with<Particle>()
            .without<IsPinned>()
            .kind(0)
            .each(flow::assemble_momentum);

        assemble_external_force = ecs.system<const Mass>("Assemble External Force")
            .with<Particle>()
            .without<IsPinned>()
            .kind(0)
            .each(flow::assemble_external_force);

        assemble_spring_force = ecs.system("Assemble Spring Force")
            .kind(0)
            .run(flow::assemble_spring_force);

        assemble_inertia = ecs.system<const Mass>("Assemble Inertia")
            .with<Particle>()
            .without<IsPinned>()
            .kind(0)
            .each(flow::assemble_inertia);

        assemble_spring_stiffness = ecs.system("Assemble Spring Stiffness")
            .kind(0)
            .run(flow::assemble_spring_stiffness);

        solve = ecs.system("Solve")
            .kind(0)
            .run(flow::solve);

        update_velocity = ecs.system<Velocity>("Update Velocity")
            .with<Particle>()
            .without<IsPinned>()
            .kind(0)
            .each(flow::update_velocity);

        integrate_position = ecs.system<Position, const Velocity>("Integrate Position")
            .with<Particle>()
            .without<IsPinned>()
            .kind(0)
            .each(flow::integrate_position);
    });

    // -------------------------------------------------------------------------

    // no scatter -- per-entity systems write to ECS directly

    ecs.system("Stats")
        .kind(sim::Simulate)
        .run(flow::stats);

    sim::load_scene(ecs, "assets/spring3.flecs");

    ecs.app()
        .enable_rest()
        .enable_stats()
        .run();

    printf("[%s] Simulation has ended.\n", __FILE__);
    return 0;
}
