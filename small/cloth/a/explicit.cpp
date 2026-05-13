// Mass-Spring Simulator (explicit euler, per-entity)

#include "../setup.h"
#include "../math/spring.h"
#include <cstdio>
#include <string>

namespace flow {

inline void clear_force(flecs::iter&, size_t, Force& f) {
    f.map().setZero();
}

inline void apply_spring_force(Spring& spring, Real stiffness, Real damping) {
    const auto x_a = spring.v0.get<Position>().map();
    const auto x_b = spring.v1.get<Position>().map();
    const auto v_a = spring.v0.get<Velocity>().map();
    const auto v_b = spring.v1.get<Velocity>().map();

    physics::spring::Eval e;
    if (!physics::spring::eval(x_a, x_b, v_a, v_b, spring.rest_length, e)) return;

    const auto g = physics::spring::grad(stiffness, damping, e);
    if (!spring.v0.has<IsPinned>()) spring.v0.get_mut<Force>().map() -= g;
    if (!spring.v1.has<IsPinned>()) spring.v1.get_mut<Force>().map() += g;
}

inline void apply_spring_elastic_force(flecs::iter&, size_t, Spring& spring) {
    apply_spring_force(spring, spring.stiffness, 0);
}

inline void apply_spring_damping_force(flecs::iter&, size_t, Spring& spring) {
    apply_spring_force(spring, 0, spring.damping);
}

inline void integrate_position(flecs::iter& it, size_t, Position& x, const Velocity& v) {
    x.map() += it.delta_time() * v.map();
}

inline void integrate_velocity(flecs::iter& it, size_t, Velocity& v, const Force& f, const Mass& m) {
    v.map() += it.delta_time() * (f.map() / m);
}

} // namespace flow

// =========================================================================
// main
// =========================================================================

int main() {
    printf("Hi from %s\n", __FILE__);

    flecs::world ecs;

    ecs.import<cloth::particle>();
    ecs.import<cloth::element>();
    ecs.import<cloth::bridge>();
    graphics::init(ecs, {800, 600, "Explicit Euler"});
    ecs.import<cloth::render>();
    ecs.import<cloth::interaction>();

    ecs.observer<Particle>()
        .event(flecs::OnAdd)
        .each([](flecs::entity e, const Particle&) {
            e.add<Force>();
        });

    // =========================================================================
    // simulation pipeline
    // =========================================================================

    flecs::system clear_force, apply_gravity;
    flecs::system apply_spring_elastic_force, apply_spring_damping_force;
    flecs::system integrate_position, integrate_velocity;

    auto algorithm = ecs.system("Explicit Euler")
        .kind(flecs::PreUpdate)
        .run([&](flecs::iter&) {
            const Real dt = props::dt.get<Real>();
            clear_force.run(dt);
            apply_gravity.run(dt);
            apply_spring_elastic_force.run(dt);
            apply_spring_damping_force.run(dt);
            integrate_velocity.run(dt);
            integrate_position.run(dt);
        });

    ecs.scope(algorithm, [&] {
        clear_force = ecs.system<Force>("Clear Force")
            .with<Particle>()
            .without<IsPinned>()
            .kind(0)
            .each(flow::clear_force);

        apply_gravity = ecs.system<Force, const Mass>("Apply Gravity")
            .with<Particle>()
            .without<IsPinned>()
            .kind(0)
            .each([](flecs::iter&, size_t, Force& f, const Mass& m) {
                f.map() += m * props::gravity.get<vec3r>().map();
            });

        apply_spring_elastic_force = ecs.system<Spring>("Apply Spring Elastic Force")
            .kind(0)
            .each(flow::apply_spring_elastic_force);

        apply_spring_damping_force = ecs.system<Spring>("Apply Spring Damping Force")
            .kind(0)
            .each(flow::apply_spring_damping_force);

        integrate_velocity = ecs.system<Velocity, const Force, const Mass>("Integrate Velocity")
            .with<Particle>()
            .without<IsPinned>()
            .kind(0)
            .each(flow::integrate_velocity);

        integrate_position = ecs.system<Position, const Velocity>("Integrate Position")
            .with<Particle>()
            .without<IsPinned>()
            .kind(0)
            .each(flow::integrate_position);
    });

    // =========================================================================
    // scene
    // =========================================================================

    sim::load_scene(ecs, "assets/spring3.flecs");

    ecs.app()
        .enable_rest()
        .enable_stats()
        .run();

    printf("[%s] Simulation has ended.\n", __FILE__);
    return 0;
}
