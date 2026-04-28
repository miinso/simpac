// Mass-Spring Simulator (symplectic euler)

#include "../setup.h"
#include "../math/spring.h"

// =========================================================================
// flow
// =========================================================================

namespace flow {

inline void clear_forces(flecs::iter&) {
    sim::state_0.clear_forces();
}

inline void apply_gravity(flecs::iter&) {
    for (int i : sim::model.free_particles)
        sim::state_0.f(i) += sim::model.particle_mass[i] * sim::gravity;
}

inline void apply_spring_force(flecs::iter&) {
    for (int s = 0; s < sim::model.spring_count; s++) {
        const int i = sim::model.spring_indices[s * 2];
        const int j = sim::model.spring_indices[s * 2 + 1];

        physics::spring::Eval e;
        if (!physics::spring::eval(sim::state_0.q(i), sim::state_0.q(j),
                                   sim::state_0.qd(i), sim::state_0.qd(j),
                                   sim::model.spring_rest_length[s], e)) continue;

        const auto g = physics::spring::grad(sim::model.spring_stiffness[s],
                                             sim::model.spring_damping[s], e);
        if (sim::model.particle_inv_mass[i] > 0) sim::state_0.f(i) -= g;
        if (sim::model.particle_inv_mass[j] > 0) sim::state_0.f(j) += g;
    }
}

inline void integrate_velocity(flecs::iter& it) {
    const Real dt = it.delta_time();
    for (int i : sim::model.free_particles)
        sim::state_0.qd(i) += dt * sim::model.particle_inv_mass[i] * sim::state_0.f(i);
}

inline void integrate_position(flecs::iter& it) {
    const Real dt = it.delta_time();
    for (int i : sim::model.free_particles)
        sim::state_0.q(i) += dt * sim::state_0.qd(i);
}

} // namespace flow

// =========================================================================
// main
// =========================================================================

int main() {
    printf("Hi from %s\n", __FILE__);

    flecs::world ecs;

    sim::init(ecs, {800, 600, "Symplectic Euler"});
    sim::install(ecs);

    // -- symplectic euler -----------------------------------------------------

    flecs::system clear_forces, apply_gravity, apply_spring_force;
    flecs::system integrate_velocity, integrate_position;

    auto integrator = ecs.system("Symplectic Euler")
        .kind(sim::Simulate)
        .run([&](flecs::iter&) {
            const Real dt = props::dt.get<Real>();
            sim::gravity = props::gravity.get<vec3f>().map();
            clear_forces.run(dt);
            apply_gravity.run(dt);
            apply_spring_force.run(dt);
            integrate_velocity.run(dt);
            integrate_position.run(dt);
        });

    ecs.scope(integrator, [&] {
        clear_forces = ecs.system("Clear Forces")
            .kind(0)
            .run(flow::clear_forces);

        apply_gravity = ecs.system("Apply Gravity")
            .kind(0)
            .run(flow::apply_gravity);

        apply_spring_force = ecs.system("Apply Spring Force")
            .kind(0)
            .run(flow::apply_spring_force);

        integrate_velocity = ecs.system("Integrate Velocity")
            .kind(0)
            .run(flow::integrate_velocity);

        integrate_position = ecs.system("Integrate Position")
            .kind(0)
            .run(flow::integrate_position);
    });

    // -------------------------------------------------------------------------

    sim::install_scatter(ecs);

    sim::load_scene(ecs, "assets/spring3.flecs");

    ecs.app()
        .enable_rest()
        .enable_stats()
        .run();

    printf("[%s] Simulation has ended.\n", __FILE__);
    return 0;
}
