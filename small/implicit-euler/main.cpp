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

    ecs.set<Scene>({});
    ecs.set<InteractionState>({});
    queries::init(ecs);

    auto& solver = ecs.ensure<Solver>();
    solver.b.setZero();
    solver.A.setZero();
    solver.cg_tolerance = 1e-3;
    solver.cg_max_iter = 100;

    // =========================================================================
    // Install systems (graphics + rendering)
    // =========================================================================

    systems::install_graphics(ecs);
    systems::install_render_systems(ecs);

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
            const auto& scene = it.world().get<Scene>();
            systems::collect_external_force(m, idx, scene.gravity, it.delta_time(), solver);
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
            systems::solve(solver);
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

    // collect system DOF - ensures solver is correctly sized
    // runs before simulation, reassigns particle indices if topology changed
    ecs.system("CollectSystemDOF")
        .kind(flecs::PreUpdate)
        .run([](flecs::iter& it) {
            auto world = it.world();
            auto& solver = world.get_mut<Solver>();

            int n = queries::num_particles();
            int dof = n * 3;

            // TODO: verbose and ugly do refactor or drop ParticleIndex completely
            bool needs_reindex = (solver.b.size() != dof);
            if (!needs_reindex) {
                queries::particle_query.each([&](flecs::entity e, const Position&) {
                    if (!e.has<ParticleIndex>()) {
                        needs_reindex = true;
                    }
                });
            }

            if (needs_reindex) {
                // assign sequential indices to all particles
                int new_idx = 0;
                queries::particle_query.each([&](flecs::entity e, const Position&) {
                    auto& idx = e.ensure<ParticleIndex>();
                    idx = new_idx++;
                });

                // resize solver
                solver.b.resize(dof);
                solver.b.setZero();
                solver.x.setZero(dof);
                solver.x_prev.setZero(dof);
                solver.A.resize(dof, dof);

                printf("[Solver] Resized: %d particles, %d DOF\n", n, dof);
            }

            // TODO: should reset spring/particle renderer bc ordering might not be preserved

            solver.b.setZero();
            solver.triplets.clear();
        });

    ecs.system("ImplicitEuler") // main simulation loop
        .kind(flecs::PreUpdate)
        .run([&](flecs::iter& it) {
            auto& scene = it.world().get_mut<Scene>();
            if (scene.paused) return;
            Real dt = scene.dt;

            // accumulate simulation time and frame count
            scene.sim_time += dt;
            scene.frame_count++;

            // build rhs, b
            // RHS = M * v_n + h * f_gravity - h * dE_dx
            collect_momentum.run(); // M * v_n
            collect_external_force.run(dt); // h * m * g
            collect_spring_gradient.run(dt); // h * -dE_dx (elastic forces)

            // build lhs, A
            // LHS = M + h^2 * ddE_ddx  (mass matrix + h^2 * Hessian)
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
