#include "components.h"
#include "systems.h"
#include "flecs.h"
#include "graphics.h"
#include <iostream>
#include <vector>

float randf(int n) {
    return static_cast<float>(rand() % n);
}

int main() {
    flecs::world ecs;
    ecs.set_threads(16);

    graphics::init(ecs);
    graphics::init_window(800, 800, "PBDSimulation");

    Vector3r gravity(0, -9.81f, 0);
    ecs.set<Scene>({0.005f, 2, 2, gravity, 0.0f});

    std::vector<flecs::entity> particles;
    int num_particles = 1000;

    for (int i = 0; i < num_particles; ++i) {
        Vector3r position((randf(200) / 100.0f - 1.0f) * 1.0f,
                          (randf(200) / 100.0f - 1.0f) * 1.0f + 2,
                          (randf(200) / 100.0f - 1.0f) * 1.0f);

        Vector3r velocity((randf(200) / 100.0f - 1.0f) * 1.0f,
                          (randf(200) / 100.0f - 1.0f) * 1.0f,
                          (randf(200) / 100.0f - 1.0f) * 1.0f);

        auto p = systems::add_particle(ecs, position, 1.0f);
        p.get_mut<Velocity>().value = velocity * 10;
        particles.push_back(p);
    }

    float stiffness = 1e+3;
    for (size_t i = 0; i < particles.size(); ++i) {
        for (size_t j = i + 1; j < particles.size(); ++j) {
            systems::add_distance_constraint(ecs, particles[i], particles[j], stiffness);
        }
    }

    // =========================================================================
    // Simulation systems (multi-threaded, manually run via parent system)
    // =========================================================================

    auto clear_acc = ecs.system<Acceleration>("ClearAcceleration")
        .with<Particle>()
        .kind(0)
        .multi_threaded()
        .each([&](Acceleration& a) {
            systems::clear_acceleration(a, gravity);
        });

    auto integrate = ecs.system<Position, OldPosition, Mass, Velocity, Acceleration>("Integrate")
        .with<Particle>()
        .kind(0)
        .multi_threaded()
        .each([](flecs::iter& it, size_t, Position& x, OldPosition& x_old,
                 Mass& mass, Velocity& v, Acceleration& a) {
            systems::integrate(x, x_old, v, mass, a, it.delta_time());
        });

    auto clear_lambda = ecs.system<DistanceConstraint>("ClearLambda")
        .kind(0)
        .multi_threaded()
        .each([](DistanceConstraint& c) {
            systems::clear_lambda(c);
        });

    auto solve_constraint = ecs.system<DistanceConstraint>("SolveConstraint")
        .kind(0)
        .multi_threaded()
        .each([](flecs::iter& it, size_t, DistanceConstraint& c) {
            systems::solve_constraint(c, it.delta_time());
        });

    auto ground_collision = ecs.system<Position, const OldPosition>("GroundCollision")
        .with<Particle>()
        .kind(0)
        .multi_threaded()
        .each([](flecs::iter& it, size_t, Position& x, const OldPosition& x_old) {
            systems::ground_collision(x, x_old, it.delta_time());
        });

    auto update_velocity = ecs.system<const Position, const OldPosition, Velocity>("UpdateVelocity")
        .with<Particle>()
        .kind(0)
        .multi_threaded()
        .each([](flecs::iter& it, size_t, const Position& x, const OldPosition& x_old, Velocity& v) {
            systems::update_velocity(v, x, x_old, it.delta_time());
        });

    // =========================================================================
    // Parent system - orchestrates simulation with fixed timestep
    // =========================================================================

    // auto sim_tick = ecs.timer().interval(1.0f / 60.0f);
    auto sim_tick = ecs.timer().interval(ecs.get<Scene>().timestep);

    ecs.system("PBDSimulation")
        .kind(flecs::PreUpdate)
        .tick_source(sim_tick)
        .run([&](flecs::iter& it) {
            const auto& scene = it.world().get<Scene>();
            Real sub_dt = scene.timestep / scene.num_substeps;

            for (int i = 0; i < scene.num_substeps; ++i) {
                clear_acc.run();
                integrate.run(sub_dt);

                clear_lambda.run();
                for (int j = 0; j < scene.solve_iter; ++j) {
                    solve_constraint.run(sub_dt);
                }

                ground_collision.run(sub_dt);
                update_velocity.run(sub_dt);
            }

            it.world().get_mut<Scene>().elapsed += scene.timestep;
        });

    // =========================================================================
    // Rendering systems
    // =========================================================================

    ecs.system<Position>("DrawParticle")
        .with<Particle>()
        .kind(graphics::phase_on_render)
        .each([](Position& x) {
            systems::draw_particle(x);
        });

    auto q_particle = ecs.query_builder().with<Particle>().cached().build();
    auto q_constraint = ecs.query_builder().with<Constraint>().cached().build();

    ecs.system("DrawStats")
        .kind(graphics::phase_post_render)
        .run([&](flecs::iter& it) {
            auto constraint_count = q_constraint.count();
            auto particle_count = q_particle.count();
            const auto& scene = it.world().get<Scene>();

            char buffer[64];
            DrawText((sprintf(buffer, "Elapsed time: %.2f", scene.elapsed), buffer), 20, 100, 20, DARKGREEN);
            DrawText((sprintf(buffer, "Particles: %d", (int)particle_count), buffer), 20, 130, 20, DARKGREEN);
            DrawText((sprintf(buffer, "Constraints: %d", (int)constraint_count), buffer), 20, 160, 20, DARKGREEN);
        });

    // =========================================================================
    // Main loop
    // =========================================================================

    graphics::run_main_loop([]{});

    std::cout << "Simulation ended." << std::endl;
    return 0;
}
