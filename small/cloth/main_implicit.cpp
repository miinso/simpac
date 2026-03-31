// Mass-Spring Simulator (implicit euler, state buffer)

#include "flecs.h"
#include "graphics.h"

#include "components.h"
#include "physics/bridge.h"
#include "physics/spring.h"
#include "queries.h"
#include "systems.h"
#include <cstdio>
#include <string>

namespace props {
inline flecs::entity cg_max_iter;
inline flecs::entity cg_tolerance;
} // namespace props

namespace sim {
inline flecs::entity Simulate;
inline physics::Model model;
inline physics::State state_0;
inline physics::Bridge bridge;
inline bool model_dirty = true;

// per-frame values, set by integrator before sub-systems
inline Eigen::Vector3r gravity = Eigen::Vector3r::Zero();

inline void seed_phases(flecs::world& ecs) {
    Simulate = ecs.entity("sim::Simulate")
        .add(flecs::Phase)
        .depends_on(flecs::PreUpdate);
}
} // namespace sim

// =========================================================================
// flow -- system callbacks
// =========================================================================

namespace flow {

// -- infrastructure --

inline void rebuild(flecs::iter& it) {
    if (!sim::model_dirty) return;
    auto world = it.world();
    world.defer_suspend();
    sim::model = sim::bridge.build(world);
    sim::state_0 = sim::model.state();
    auto& solver = world.ensure<Solver>();
    solver.b.resize(0);
    solver.x.resize(0);
    solver.x_prev.resize(0);
    solver.exploded = false;
    sim::model_dirty = false;
    world.defer_resume();
    printf("[Solver] rebuilt: %d particles, %d springs\n",
           sim::model.particle_count, sim::model.spring_count);
}

inline void gather(flecs::iter&) {
    sim::bridge.gather(sim::state_0);
}

// -- implicit euler phases --

inline void prepare(flecs::iter& it) {
    auto& solver = it.world().ensure<Solver>();
    const int dof = sim::model.particle_count * 3;
    if (solver.b.size() != dof) {
        solver.b.resize(dof);
        solver.x.resize(dof);
        solver.x_prev.resize(dof);
        solver.A.resize(dof, dof);
        solver.x.setZero();
        solver.x_prev.setZero();
    }
    if (solver.x.size() > 0 && (!solver.x.allFinite() || !solver.x_prev.allFinite()))
        solver.exploded = true;
    if (solver.exploded) {
        solver.x.setZero();
        solver.x_prev.setZero();
        solver.exploded = false;
    }
    solver.b.setZero();
    solver.triplets.clear();
}

inline void collect_momentum(flecs::iter&, size_t,
                             const Mass& m, const Velocity& v,
                             const ParticleIndex& idx, Solver& solver) {
    solver.b.segment<3>(idx * 3) += (Real)m * v.map();
}

inline void collect_external_force(flecs::iter& it, size_t,
                                   const Mass& m,
                                   const ParticleIndex& idx, Solver& solver) {
    solver.b.segment<3>(idx * 3) += it.delta_time() * (Real)m * sim::gravity;
}

inline void collect_spring_gradient(flecs::iter& it) {
    auto& solver = it.world().ensure<Solver>();
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
        if (sim::model.particle_inv_mass[i] > Real(0)) solver.b.segment<3>(i * 3) -= dt * g;
        if (sim::model.particle_inv_mass[j] > Real(0)) solver.b.segment<3>(j * 3) += dt * g;
    }
}

inline void collect_mass(flecs::iter&, size_t,
                         const Mass& m,
                         const ParticleIndex& idx, Solver& solver) {
    for (int d = 0; d < 3; d++)
        solver.triplets.push_back({idx * 3 + d, idx * 3 + d, (Real)m});
}

inline void collect_spring_hessian(flecs::iter& it) {
    auto& solver = it.world().ensure<Solver>();
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
        const bool i_free = sim::model.particle_inv_mass[i] > Real(0);
        const bool j_free = sim::model.particle_inv_mass[j] > Real(0);

        for (int r = 0; r < 3; r++) {
            for (int c = 0; c < 3; c++) {
                const Real val = h2 * H(r, c);
                if (i_free) solver.triplets.push_back({i*3+r, i*3+c, val});
                if (j_free) solver.triplets.push_back({j*3+r, j*3+c, val});
                if (i_free && j_free) {
                    solver.triplets.push_back({i*3+r, j*3+c, -val});
                    solver.triplets.push_back({j*3+r, i*3+c, -val});
                }
            }
        }
    }
}

inline void solve(flecs::iter& it) {
    auto& solver = it.world().ensure<Solver>();

    solver.A.setFromTriplets(solver.triplets.begin(), solver.triplets.end());
    solver.cg.setMaxIterations(props::cg_max_iter.get<int>());
    solver.cg.setTolerance(props::cg_tolerance.get<Real>());
    solver.cg.compute(solver.A);

    solver.x = solver.cg.solveWithGuess(solver.b, solver.x_prev);
    solver.x_prev = solver.x;
    solver.cg_iterations = (int)solver.cg.iterations();
    solver.cg_error = solver.cg.error();

    if (solver.cg.info() != Eigen::Success || !solver.x.allFinite())
        solver.exploded = true;
}

inline void update_velocity(flecs::iter&, size_t,
                            Velocity& v,
                            const ParticleIndex& idx, const Solver& solver) {
    if (solver.exploded) return;
    v.map() = solver.x.segment<3>(idx * 3);
}

inline void integrate_position(flecs::iter& it, size_t,
                               Position& pos, const Velocity& v) {
    pos.map() += it.delta_time() * v.map();
}

inline void stats(flecs::iter& it) {
    const auto& solver = it.world().get<Solver>();

    char buf[128];
    snprintf(buf, sizeof(buf), "CG: %d iter, err=%e%s",
             solver.cg_iterations, solver.cg_error,
             solver.exploded ? " FAILED" : "");

    const int frame = state::frame_count.get<int>();
    if (solver.exploded || (frame % 50 == 0)) {
        auto& ui = it.world().ensure<Solver>();
        ui.cg_history.push_back(buf);
        if (ui.cg_history.size() > (size_t)ui.cg_history_max_lines)
            ui.cg_history.pop_front();
    }
    if (solver.exploded) printf("%s\n", buf);
}

} // namespace flow

// =========================================================================
// Main
// =========================================================================

int main() {
    printf("Hi from %s\n", __FILE__);

    flecs::world ecs;

    // 1) Types
    components::register_core_components(ecs);
    components::register_solver_component(ecs);
    components::register_render_components(ecs);

    // 2) Runtime data
    queries::seed(ecs);
    props::seed(ecs);
    state::seed(ecs);
    sim::seed_phases(ecs);

    props::dt.set<Real>(Real(1.0f / 60.0f));
    props::gravity.set<vec3f>({0.0f, -9.81f, 0.0f});
    props::paused.set<bool>(false);

    props::cg_max_iter = ecs.entity("Config::Solver::cg_max_iter").set<int>(100).add<Configurable>();
    props::cg_tolerance = ecs.entity("Config::Solver::cg_tolerance").set<Real>(Real(1e-3f)).add<Configurable>();

    ecs.ensure<Solver>();
    ecs.set<ParticleInteractionState>({});

    // 3) Services
    graphics::init(ecs, {800, 600, "Base Simulator"});

    ecs.set<SpringRenderer>({});
    ecs.set<ParticleRenderer>({});

    // 4) Systems
    systems::install_scene_systems(ecs);
    systems::install_render_systems(ecs);

    // =========================================================================
    // Observers
    // =========================================================================

    ecs.observer<Particle>()
        .event(flecs::OnAdd)
        .each([](flecs::entity e, const Particle&) {
            e.add<ParticleState>();
        });

    ecs.observer<Particle>()
        .event(flecs::OnAdd)
        .event(flecs::OnRemove)
        .run([](flecs::iter&) { sim::model_dirty = true; });

    ecs.observer<Spring>()
        .event(flecs::OnSet)
        .event(flecs::OnAdd)
        .event(flecs::OnRemove)
        .each([](flecs::iter&, size_t, Spring&) { sim::model_dirty = true; });

    // =========================================================================
    // Simulation pipeline
    // =========================================================================

    ecs.system("Rebuild")
        .kind(sim::Simulate)
        .run(flow::rebuild);

    ecs.system("Gather")
        .kind(sim::Simulate)
        .run(flow::gather);

    // -- Implicit Euler --

    flecs::system prepare, collect_momentum, collect_external_force;
    flecs::system collect_spring_gradient, collect_mass, collect_spring_hessian;
    flecs::system solve, update_velocity, integrate_position;

    auto integrator = ecs.system("Implicit Euler")
        .kind(sim::Simulate)
        .run([&](flecs::iter&) {
            const Real dt = props::dt.get<Real>();
            sim::gravity = props::gravity.get<vec3f>().map();
            prepare.run(dt);
            collect_momentum.run(dt);
            collect_external_force.run(dt);
            collect_spring_gradient.run(dt);
            collect_mass.run(dt);
            collect_spring_hessian.run(dt);
            solve.run(dt);
            update_velocity.run(dt);
            integrate_position.run(dt);
        });

    ecs.scope(integrator, [&] {
        prepare = ecs.system("Prepare")
            .kind(0)
            .run(flow::prepare);

        collect_momentum = ecs.system<const Mass, const Velocity, const ParticleIndex, Solver>("Collect Momentum")
            .with<Particle>()
            .without<IsPinned>()
            .kind(0)
            .each(flow::collect_momentum);

        collect_external_force = ecs.system<const Mass, const ParticleIndex, Solver>("Collect External Force")
            .with<Particle>()
            .without<IsPinned>()
            .kind(0)
            .each(flow::collect_external_force);

        collect_spring_gradient = ecs.system("Collect Spring Gradient")
            .kind(0)
            .run(flow::collect_spring_gradient);

        collect_mass = ecs.system<const Mass, const ParticleIndex, Solver>("Collect Mass")
            .with<Particle>()
            .without<IsPinned>()
            .kind(0)
            .each(flow::collect_mass);

        collect_spring_hessian = ecs.system("Collect Spring Hessian")
            .kind(0)
            .run(flow::collect_spring_hessian);

        solve = ecs.system("Solve")
            .kind(0)
            .run(flow::solve);

        update_velocity = ecs.system<Velocity, const ParticleIndex, const Solver>("Update Velocity")
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

    // -- end integrator --

    ecs.system("Stats")
        .kind(sim::Simulate)
        .run(flow::stats);

    ecs.system("graphics::DrawSolveHistory")
        .kind(graphics::PostRender)
        .run(render::draw_solve_history)
        .disable(0);

    // =========================================================================
    // Load scene script
    // =========================================================================

    const std::string scene_path = graphics::npath("assets/spring3.flecs");
    if (!scene_path.empty()) {
        auto script = ecs.script("SceneScript").filename(scene_path.c_str()).run();
        if (!script) {
            std::printf("[Scene] Failed to load %s\n", scene_path.c_str());
        } else if (const EcsScript* data = script.try_get<EcsScript>(); data && data->error) {
            std::printf("[Scene] Script error for %s: %s\n", scene_path.c_str(), data->error);
        } else {
            std::printf("[Scene] Loaded %s\n", scene_path.c_str());
        }
    }

    // =========================================================================
    // Main loop
    // =========================================================================

    ecs.app()
        .enable_rest()
        .enable_stats()
        .run();

    printf("[%s] Simulation has ended.\n", __FILE__);
    return 0;
}
