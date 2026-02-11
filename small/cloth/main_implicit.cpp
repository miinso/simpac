// Mass-Spring Simulator Base (implicit euler)

#include "flecs.h"
#include "graphics.h"

#include "components.h"
#include "physics.h"
#include "queries.h"
#include "systems.h"
#include <cstdio>
#include <string>

// user-defined props for implicit path
namespace props {
inline flecs::entity cg_max_iter;
inline flecs::entity cg_tolerance;
} // namespace props

namespace flow {

inline void prepare_solver(flecs::iter& it, const flecs::entity& scene_dirty) {
    auto world = it.world();
    auto& solver = world.get_mut<Solver>();

    int n = queries::num_particles();
    int dof = n * 3;

    bool resized = solver.b.size() != dof;
    if (resized) {
        solver.b.resize(dof);
        solver.x.resize(dof);
        solver.x_prev.resize(dof);
        solver.A.resize(dof, dof);
        printf("[Solver] Resized: %d particles, %d DOF\n", n, dof);
    }

    bool invalid_state = (solver.x.size() > 0)
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

inline void collect_momentum(flecs::iter& it, size_t, const Mass& m, const Velocity& v,
                             const ParticleIndex& idx, Solver& solver) {
    auto momentum = m * v.map();
    solver.b.segment<3>(idx * 3) += momentum;
}

inline void collect_external_force(flecs::iter& it, size_t, const Mass& m,
                                   const ParticleIndex& idx, Solver& solver,
                                   const vec3f& gravity) {
    auto f_gravity = m * gravity.map();
    solver.b.segment<3>(idx * 3) += it.delta_time() * f_gravity;
}

inline void collect_spring_gradient(flecs::iter& it, size_t, Spring& spring, Solver& solver) {
    physics::spring::Sample sample;
    if (!physics::spring::sample(spring, sample, true)) return;

    auto grad_a = physics::spring::grad(spring.k_s, spring.k_d, sample.eval);
    auto grad_b = -grad_a;

    if (!sample.a_pinned) solver.b.segment<3>(sample.idx_a * 3) += -it.delta_time() * grad_a;
    if (!sample.b_pinned) solver.b.segment<3>(sample.idx_b * 3) += -it.delta_time() * grad_b;
}

inline void collect_mass(flecs::iter& it, size_t, const Mass& m, const ParticleIndex& idx, Solver& solver) {
    for (int i = 0; i < 3; ++i) {
        solver.triplets.push_back({idx * 3 + i, idx * 3 + i, m});
    }
}

inline void collect_spring_hessian(flecs::iter& it, size_t, Spring& spring, Solver& solver) {
    physics::spring::Sample sample;
    if (!physics::spring::sample(spring, sample, true)) return;

    Eigen::Matrix3r H_block = physics::spring::hess(spring.k_s, spring.rest_length, sample.eval);
    Real h2 = it.delta_time() * it.delta_time();

    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            Real val = h2 * H_block(r, c);
            if (!sample.a_pinned) solver.triplets.push_back({sample.idx_a * 3 + r, sample.idx_a * 3 + c, val});
            if (!sample.b_pinned) solver.triplets.push_back({sample.idx_b * 3 + r, sample.idx_b * 3 + c, val});
            if (!sample.a_pinned && !sample.b_pinned) {
                solver.triplets.push_back({sample.idx_a * 3 + r, sample.idx_b * 3 + c, -val});
                solver.triplets.push_back({sample.idx_b * 3 + r, sample.idx_a * 3 + c, -val});
            }
        }
    }
    // NOTE: sparsemat assembly is costly. do hand-rolled cg
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
    if (!cg_success
        || !solver.x.allFinite()
        || !solver.x_prev.allFinite()) {
        solver.exploded = true;
    }

    char buf[128];
    snprintf(buf, sizeof(buf), "CG: %d iter, err=%e%s",
             solver.cg_iterations,
             solver.cg_error,
             !cg_success ? " FAILED" : "");

    if (!cg_success) {
        printf("%s\n", buf);
    }

    const int frame_count = state::frame_count.get<int>();
    if (!cg_success || (frame_count % 50 == 0)) {
        solver.cg_history.push_back(buf);
        if (solver.cg_history.size() > (size_t)solver.cg_history_max_lines) {
            solver.cg_history.pop_front();
        }
    }
}

inline void update_velocity(flecs::iter& it, size_t, Velocity& v, const ParticleIndex& idx,
                            const Solver& solver) {
    v.map() = solver.x.segment<3>(idx * 3);
}

inline void integrate_position(flecs::iter& it, size_t, Position& x, const Velocity& v) {
    x.map() += it.delta_time() * v.map();
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

    // hint: override core props here
    props::dt.set<Real>(Real(1.0f / 60.0f));
    props::gravity.set<vec3f>({0.0f, -9.81f, 0.0f});
    props::paused.set<bool>(false);

    // implicit-specific configurables
    props::cg_max_iter = ecs.entity("Config::Solver::cg_max_iter").set<int>(100).add<Configurable>();
    props::cg_tolerance = ecs.entity("Config::Solver::cg_tolerance").set<Real>(Real(1e-3f)).add<Configurable>();

    // non-GL singletons
    ecs.ensure<Solver>();
    ecs.set<ParticleInteractionState>({});

    // 3) Services
    graphics::init(ecs, {800, 600, "Base Simulator"});

    // GL singletons (after graphics::init)
    ecs.set<SpringRenderer>({});
    ecs.set<ParticleRenderer>({});

    // 4) Systems
    systems::install_scene_systems(ecs);
    systems::install_render_systems(ecs);

    // some per-demo behaviors
    // reset solver warm-start when springs change
    ecs.observer<Spring>()
        .event(flecs::OnSet)
        .event(flecs::OnAdd)
        .event(flecs::OnRemove)
        .each([](flecs::iter& it, size_t, Spring&) {
            state::dirty.get_mut<bool>() = true;
        });

    // mark scene dirty when particles are added/removed
    ecs.observer<Particle>()
        .event(flecs::OnAdd)
        .each([](flecs::entity e, const Particle&) {
            e.add<ParticleState>();
        });

    // mark scene dirty when particles are added/removed
    ecs.observer<Particle>()
        .event(flecs::OnAdd)
        .event(flecs::OnRemove)
        .run([](flecs::iter& it) {
            state::dirty.get_mut<bool>() = true;
        });

    ecs.system("graphics::DrawSolveHistory")
        .kind(graphics::PostRender)
        .run(render::draw_solve_history)
        .disable(0);

    // =========================================================================
    // System handles
    // =========================================================================

    flecs::system prepare_solver;
    flecs::system collect_momentum;
    flecs::system collect_external_force;
    flecs::system collect_spring_gradient;
    flecs::system collect_mass;
    flecs::system collect_spring_hessian;
    flecs::system solve;
    flecs::system update_velocity;
    flecs::system integrate_position;

    auto algorithm = ecs.system("Implicit Euler")
        .kind(flecs::PreUpdate)
        .run([&](flecs::iter& it) {
            const auto& dt = props::dt.get<Real>();

            prepare_solver.run(dt);

            if (collect_momentum.enabled()) collect_momentum.run(dt);
            if (collect_external_force.enabled()) collect_external_force.run(dt);
            if (collect_spring_gradient.enabled()) collect_spring_gradient.run(dt);
            if (collect_mass.enabled()) collect_mass.run(dt);
            if (collect_spring_hessian.enabled()) collect_spring_hessian.run(dt);
            if (solve.enabled()) solve.run();
            if (update_velocity.enabled()) update_velocity.run(dt);
            if (integrate_position.enabled()) integrate_position.run(dt);
        });

    // =========================================================================
    // Flow block (order = flow)
    // =========================================================================
    ecs.scope(algorithm, [&] {
        prepare_solver = ecs.system("Prepare Solver")
            .kind(0)
            .run([](flecs::iter& it) {
                flow::prepare_solver(it, state::dirty);
            });

        collect_momentum = ecs.system<const Mass, const Velocity, const ParticleIndex, Solver>("Collect Momentum")
            .with<Particle>()
            .without<IsPinned>()
            .kind(0)
            .each(flow::collect_momentum);

        collect_external_force = ecs.system<const Mass, const ParticleIndex, Solver>("Collect External Force")
            .with<Particle>()
            .without<IsPinned>()
            .kind(0)
            .each([](flecs::iter& it, size_t i, const Mass& m, const ParticleIndex& idx, Solver& solver) {
                flow::collect_external_force(it, i, m, idx, solver, props::gravity.get<vec3f>());
            });

        collect_spring_gradient = ecs.system<Spring, Solver>("Collect Spring Gradient")
            .with<Spring>()
            .kind(0)
            .each(flow::collect_spring_gradient);

        collect_mass = ecs.system<const Mass, const ParticleIndex, Solver>("Collect Mass")
            .with<Particle>()
            .without<IsPinned>()
            .kind(0)
            .each(flow::collect_mass);

        collect_spring_hessian = ecs.system<Spring, Solver>("Collect Spring Hessian")
            .with<Spring>()
            .kind(0)
            .each(flow::collect_spring_hessian);

        solve = ecs.system("Solve Linear System")
            .kind(0)
            .run([](flecs::iter& it) {
                flow::solve(it, props::cg_max_iter.get<int>(), props::cg_tolerance.get<Real>());
            });

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

