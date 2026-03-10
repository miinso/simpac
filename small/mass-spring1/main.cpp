// PBD Mass-Spring Simulator
// Parent system pattern with explicit .run() calls

#include "components.h"
#include "systems.h"
#include "flecs.h"
#include "graphics.h"
#include <cstdio>

// =========================================================================
// Helper functions
// =========================================================================

flecs::entity add_particle(flecs::world& ecs, const Vector3r& pos, Real mass = 1.0) {
    auto e = ecs.entity();
    e.add<Particle>()
        .set<Position>({pos})
        .set<OldPosition>({pos})
        .set<Velocity>({Vector3r::Zero()})
        .set<Acceleration>({Vector3r::Zero()})
        .set<Mass>({mass})
        .set<InverseMass>({1.0 / mass});
    return e;
}

flecs::entity add_distance_constraint(flecs::world& ecs, flecs::entity& e1, flecs::entity& e2, Real stiffness = 1.0) {
    const auto& p1 = e1.get<Position>();
    const auto& p2 = e2.get<Position>();
    auto rest_distance = (p1.value - p2.value).norm();

    auto e = ecs.entity();
    e.add<Constraint>()
        .set<DistanceConstraint>({e1, e2, rest_distance, stiffness, 0.0});
    return e;
}

// =========================================================================
// Main
// =========================================================================

int main() {
    printf("Hi from %s\n", __FILE__);

    flecs::world ecs;

    // Scene setup
    Vector3r gravity(0, -9.81, 0);
    ecs.set<Scene>({
        0.01,       // dt (timestep)
        100,        // num_substeps
        10,         // solve_iter
        gravity     // gravity
        // rest are default initialized
    });

    // Initialize graphics
    graphics::init(ecs, {800, 600, "PBD Mass-Spring"});

    // Create initial particles with random masses
    auto p1 = add_particle(ecs, Vector3r(0, 4, 0), 1.0);
    auto p2 = add_particle(ecs, Vector3r(-1, 4, 0), 1.0 * std::pow(10, GetRandomValue(0, 3)));
    auto p3 = add_particle(ecs, Vector3r(-2, 4, 0), 1.0 * std::pow(10, GetRandomValue(0, 3)));
    auto p4 = add_particle(ecs, Vector3r(-3, 4, 0), 1.0 * std::pow(10, GetRandomValue(0, 3)));

    // Pin first particle
    p1.add<IsPinned>();
    p1.set<InverseMass>({0});

    // Create constraints with random stiffness
    add_distance_constraint(ecs, p1, p2, 1e+5 * std::pow(10, GetRandomValue(0, 2)));
    add_distance_constraint(ecs, p2, p3, 1e+5 * std::pow(10, GetRandomValue(0, 2)));
    add_distance_constraint(ecs, p3, p4, 1e+5 * std::pow(10, GetRandomValue(0, 2)));

    // =========================================================================
    // Simulation systems (no .kind() - called manually via .run())
    // =========================================================================

    auto clear_acc = ecs.system<Acceleration>("ClearAcceleration")
        .with<Particle>()
        .kind(0)
        .each([&](Acceleration& a) {
            systems::clear_acceleration(a, gravity);
        });

    auto integrate = ecs.system<Position, OldPosition, Velocity, const Mass, const Acceleration>("Integrate")
        .with<Particle>()
        .without<IsPinned>()
        .kind(0)
        .each([](flecs::iter& it, size_t, Position& x, OldPosition& x_old,
                 Velocity& v, const Mass& mass, const Acceleration& acc) {
            systems::integrate(x, x_old, v, mass, acc, it.delta_time());
        });

    auto clear_lambda = ecs.system<DistanceConstraint>("ClearLambda")
        .kind(0)
        .each([](DistanceConstraint& c) {
            systems::clear_lambda(c);
        });

    auto solve_constraint = ecs.system<DistanceConstraint>("SolveConstraint")
        .kind(0)
        .each([](flecs::iter& it, size_t, DistanceConstraint& c) {
            systems::solve_constraint(c, it.delta_time());
        });

    auto ground_collision = ecs.system<Position, const OldPosition>("GroundCollision")
        .with<Particle>()
        .kind(0)
        .each([](flecs::iter& it, size_t, Position& x, const OldPosition& x_old) {
            systems::ground_collision(x, x_old, it.delta_time());
        });

    auto update_velocity = ecs.system<Velocity, const Position, const OldPosition>("UpdateVelocity")
        .with<Particle>()
        .kind(0)
        .each([](flecs::iter& it, size_t, Velocity& v, const Position& x, const OldPosition& x_old) {
            systems::update_velocity(v, x, x_old, it.delta_time());
        });

    // =========================================================================
    // Rendering systems (on graphics phases - run via ecs.progress())
    // =========================================================================

    ecs.system<DistanceConstraint>("DrawConstraints")
        .kind(graphics::OnRender)
        .each([](DistanceConstraint& c) {
            systems::draw_constraint(c);
        });

    ecs.system<const Position, const Mass>("DrawParticles")
        .with<Particle>()
        .kind(graphics::OnRender)
        .each([](const Position& x, const Mass& m) {
            systems::draw_particle(x, m);
        });

    ecs.system<const Position, const Mass>("DrawMassInfo")
        .with<Particle>()
        .kind(graphics::PostRender)
        .each([](const Position& x, const Mass& m) {
            systems::draw_mass_info(x, m);
        });

    ecs.system<DistanceConstraint>("DrawConstraintLambda")
        .kind(graphics::PostRender)
        .each([](DistanceConstraint& c) {
            systems::draw_constraint_lambda(c);
        });

    ecs.system("DrawTimingInfo")
        .kind(graphics::PostRender)
        .run(systems::draw_timing_info);

    // =========================================================================
    // Parent system - THE ALGORITHM in one place
    // =========================================================================

    // Fixed 60 Hz simulation tick (decoupled from display rate)
    auto sim_tick = ecs.timer().interval(1.0f / 60.0f);

    ecs.system("PBDSimulation")
        .kind(flecs::PreUpdate)
        .tick_source(sim_tick)
        .run([&](flecs::iter& it) {
            auto& scene = it.world().get_mut<Scene>();
            Real sub_dt = scene.dt / scene.num_substeps;

            // accumulate simulation time and frame count
            scene.sim_time += scene.dt;
            scene.frame_count++;

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
        });

    // =========================================================================
    // Main loop
    // =========================================================================

    ecs.app().run();

    printf("Simulation ended.\n");
    return 0;
}
