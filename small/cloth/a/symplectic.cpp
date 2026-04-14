// Mass-Spring Simulator (symplectic euler, per-entity)

#include "flecs.h"
#include "graphics.h"

#include "../components.h"
#include "../math/spring.h"
#include "../queries.h"
#include "../systems.h"
#include <cstdio>
#include <string>

namespace flow {

inline void clear_force(flecs::iter&, size_t, Force& f) {
    f.map().setZero();
}

inline void apply_spring_force(flecs::iter&, size_t, Spring& spring) {
    const auto x_a = spring.e1.get<Position>().map();
    const auto x_b = spring.e2.get<Position>().map();
    const auto v_a = spring.e1.get<Velocity>().map();
    const auto v_b = spring.e2.get<Velocity>().map();

    physics::spring::Eval e;
    if (!physics::spring::eval(x_a, x_b, v_a, v_b, spring.rest_length, e)) return;

    const auto g = physics::spring::grad(spring.k_s, spring.k_d, e);
    if (!spring.e1.has<IsPinned>()) spring.e1.get_mut<Force>().map() -= g;
    if (!spring.e2.has<IsPinned>()) spring.e2.get_mut<Force>().map() += g;
}

// symplectic: velocity first, then position with new velocity
inline void integrate_velocity(flecs::iter& it, size_t, Velocity& v, const Force& f, const Mass& m) {
    v.map() += it.delta_time() * (f.map() / m);
}

inline void integrate_position(flecs::iter& it, size_t, Position& x, const Velocity& v) {
    x.map() += it.delta_time() * v.map();
}

} // namespace flow

// =========================================================================
// main
// =========================================================================

int main() {
    printf("Hi from %s\n", __FILE__);

    flecs::world ecs;

    components::register_particle_components(ecs);
    components::register_constraint_components(ecs);
    components::register_render_components(ecs);

    queries::seed(ecs);
    props::seed(ecs);
    state::seed(ecs);

    props::dt.set<Real>(Real(1.0f / 60.0f));
    props::gravity.set<vec3r>({0.0f, -9.81f, 0.0f});
    props::paused.set<bool>(false);

    ecs.set<ParticleInteractionState>({});

    graphics::init(ecs, {800, 600, "Symplectic Euler (ecs)"});
    ecs.set<SpringRenderer>({});
    ecs.set<ParticleRenderer>({});

    systems::install_scene_systems(ecs);
    systems::install_render_systems(ecs);

    ecs.observer<Particle>()
        .event(flecs::OnAdd)
        .each([](flecs::entity e, const Particle&) {
            e.add<Force>();
            e.add<ParticleState>();
        });

    // -- symplectic euler -----------------------------------------------------

    flecs::system clear_force, apply_gravity, apply_spring_force;
    flecs::system integrate_velocity, integrate_position;

    auto integrator = ecs.system("Symplectic Euler")
        .kind(flecs::PreUpdate)
        .run([&](flecs::iter&) {
            const Real dt = props::dt.get<Real>();
            clear_force.run(dt);
            apply_gravity.run(dt);
            apply_spring_force.run(dt);
            integrate_velocity.run(dt);
            integrate_position.run(dt);
        });

    ecs.scope(integrator, [&] {
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

        apply_spring_force = ecs.system<Spring>("Apply Spring Force")
            .kind(0)
            .each(flow::apply_spring_force);

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

    // -------------------------------------------------------------------------

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

    ecs.app()
        .enable_rest()
        .enable_stats()
        .run();

    printf("[%s] Simulation has ended.\n", __FILE__);
    return 0;
}
