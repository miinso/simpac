// Mass-Spring Simulator Base

#include "flecs.h"
#include "graphics.h"

#include "components.h"
#include "queries.h"
#include "systems.h"
#include "cloth.h"
#include <cstdio>
#include <string>

// =========================================================================
// Main
// =========================================================================

int main() {
    printf("Hi from %s\n", __FILE__);

    flecs::world ecs;

    // =========================================================================
    // Register components
    // =========================================================================

    register_component(ecs);

    // =========================================================================
    // Initialize state
    // =========================================================================

    ecs.entity("Config::Scene::dt")
        .set<Real>(Real(1.0f / 60.0f))
        .add<Configurable>();
    ecs.entity("Config::Scene::gravity")
        .set<vec3f>({0.0f, -9.81f, 0.0f})
        .add<Configurable>();
    ecs.entity("Config::Scene::paused").set<bool>(false).add<Configurable>();
    ecs.entity("Scene::wall_time").set<Real>(Real(0.0f));
    ecs.entity("Scene::sim_time").set<Real>(Real(0.0f));
    ecs.entity("Scene::frame_count").set<int>(0);
    ecs.entity("Scene::dirty").set<bool>(false);
    ecs.entity("Config::Solver::cg_max_iter").set<int>(100).add<Configurable>();
    ecs.entity("Config::Solver::cg_tolerance").set<Real>(Real(1e-3f)).add<Configurable>();
    ecs.set<InteractionState>({});
    queries::init(ecs);

    ecs.ensure<Solver>();

    // =========================================================================
    // Install systems (graphics + rendering)
    // =========================================================================

    systems::install_graphics(ecs);
    systems::install_render_systems(ecs);

    // reset solver warm-start when springs change
    ecs.observer<Spring>()
        .event(flecs::OnSet)
        .event(flecs::OnAdd)
        .event(flecs::OnRemove)
        .each([](flecs::iter& it, size_t, Spring&) {
            it.world().lookup("Scene::dirty").get_mut<bool>() = true;
        });

    // mark scene dirty when particles are added/removed
    ecs.observer<Particle>()
        .event(flecs::OnAdd)
        .event(flecs::OnRemove)
        .run([](flecs::iter& it) {
            it.world().lookup("Scene::dirty").get_mut<bool>() = true;
        });

    // =========================================================================
    // Simulation systems (we set .kind(0) to manually run them
    // =========================================================================

    auto collect_momentum = ecs.system<const Mass, const Velocity, const ParticleIndex, Solver>("Collect Momentum")
        .with<Particle>()
        .without<IsPinned>()
        .kind(0)
        .each([&](flecs::iter& it, size_t, const Mass& m, const Velocity& v, const ParticleIndex& idx, Solver& solver) {
            systems::collect_momentum(m, v, idx, solver);
        });

    auto collect_external_force = ecs.system<const Mass, const ParticleIndex, Solver>("Collect External Force")
        .with<Particle>()
        .without<IsPinned>()
        .kind(0)
        .each([&](flecs::iter& it, size_t, const Mass& m, const ParticleIndex& idx, Solver& solver) {
            const auto& gravity = it.world().lookup("Config::Scene::gravity").get<vec3f>();
            systems::collect_external_force(m, idx, gravity, it.delta_time(), solver);
        });

    auto collect_spring_gradient = ecs.system<Spring, Solver>("Collect Spring Gradient")
        .with<Spring>()
        .kind(0)
        .each([](flecs::iter& it, size_t, Spring& spring, Solver& solver) {
            systems::collect_spring_gradient(spring, it.delta_time(), solver);
        });

    auto collect_mass = ecs.system<const Mass, const ParticleIndex, Solver>("Collect Mass")
        .with<Particle>()
        .without<IsPinned>()
        .kind(0)
        .each(systems::collect_mass);

    auto collect_spring_hessian = ecs.system<Spring, Solver>("Collect Spring Hessian")
        .with<Spring>()
        .kind(0)
        .each([](flecs::iter& it, size_t, Spring& spring, Solver& solver) {
            systems::collect_spring_hessian(spring, it.delta_time(), solver);
        });
        
    auto solve = ecs.system("Solve Linear System")
        .kind(0)
        .run([](flecs::iter& it) {
            // `Solver` here is a singleton
            auto& solver = it.world().get_mut<Solver>();
            const auto& cg_max_iter = it.world().lookup("Config::Solver::cg_max_iter").get<int>();
            const auto& cg_tolerance = it.world().lookup("Config::Solver::cg_tolerance").get<Real>();
            systems::solve(solver, cg_max_iter, cg_tolerance);
        });

    auto update_velocity = ecs.system<Velocity, const ParticleIndex, const Solver>("Update Velocity")
        .with<Particle>()
        .without<IsPinned>()
        .kind(0)
        .each([](flecs::iter& it, size_t, Velocity& v, const ParticleIndex& idx, const Solver& solver) {
            systems::update_velocity(v, idx, solver);
        });

    auto update_position = ecs.system<Position, const Velocity>("Update Position")
        .with<Particle>()
        .without<IsPinned>()
        .kind(0)
        .each([](flecs::iter& it, size_t, Position& x, const Velocity& v) {
            systems::update_position(x, v, it.delta_time());
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
    // LOOK HERE!!! - THE ALGORITHM in one place
    // =========================================================================

    // scene indexing pass - assigns dense indices when topology changes
    ecs.system("Scene::ReindexParticles")
        .kind(flecs::PreUpdate)
        .run([](flecs::iter& it) {
            auto world = it.world();
            if (!world.lookup("Scene::dirty").get<bool>()) return;

            int new_idx = 0;
            queries::particle_query.each([&](flecs::entity e, const Position&) {
                e.set<ParticleIndex>(new_idx++);
            });
        });

    // solver prep - size, reset, and clear per-frame buffers
    ecs.system("Solver::PrepareFrame")
        .kind(flecs::PreUpdate)
        // .after("Scene::ReindexParticles") // error C2039: 'after': is not a member of 'flecs::system_builder<>'
        .run([](flecs::iter& it) {
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

            if (world.lookup("Scene::dirty").get<bool>() || solver.exploded || resized) {
                solver.x.setZero();
                solver.x_prev.setZero();
                solver.exploded = false;
            }

            solver.b.setZero();
            solver.triplets.clear();
        });

    ecs.system("Scene::ClearDirty")
        .kind(flecs::PreUpdate)
        .run([](flecs::iter& it) {
            it.world().lookup("Scene::dirty").get_mut<bool>() = false;
        });

    ecs.system("Implicit Euler") // main simulation loop
        .kind(flecs::PreUpdate)
        .run([&](flecs::iter& it) {
            auto& wall_time = it.world().lookup("Scene::wall_time").get_mut<Real>();
            wall_time += it.delta_time();

            if (it.world().lookup("Config::Scene::paused").get<bool>()) return;
            const auto& dt = it.world().lookup("Config::Scene::dt").get<Real>();

            // accumulate simulation time and frame count
            auto& sim_time = it.world().lookup("Scene::sim_time").get_mut<Real>();
            auto& frame_count = it.world().lookup("Scene::frame_count").get_mut<int>();
            sim_time += dt;
            frame_count += 1;

            // build rhs, b
            // RHS = M * v_n + h * f_gravity - h * dE_dx
            collect_momentum.run(); // M * v_n
            collect_external_force.run(dt); // h * m * g
            collect_spring_gradient.run(dt); // h * -dE_dx (elastic forces)

            // build lhs, A
            // LHS = M + h^2 * ddE_ddx (mass matrix + h^2 * Hessian)
            collect_mass.run(dt);
            collect_spring_hessian.run(dt);
            
            // solve for x
            solve.run(); 

            // update states
            update_velocity.run(dt);
            update_position.run(dt);
        });

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
