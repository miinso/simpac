// Mass-Spring Simulator Base (explicit euler)

#include "flecs.h"
#include "graphics.h"

#include "components.h"
#include "physics.h"
#include "queries.h"
#include "systems.h"
#include <cstdio>
#include <string>

namespace flow {

inline void clear_force(flecs::iter& it, size_t, Force& f) {
    f.map().setZero(); // .map() give you eigen handle
}

inline void apply_spring_force(Spring& spring, Real k_s, Real k_d) {
    physics::spring::Sample sample;
    Eigen::Vector3r force_a = Eigen::Vector3r::Zero();
    Eigen::Vector3r force_b = Eigen::Vector3r::Zero();
    if (!physics::spring::force(spring, k_s, k_d, sample, force_a, force_b)) return;

    if (!sample.a_pinned) sample.a.get_mut<Force>().map() += force_a;
    if (!sample.b_pinned) sample.b.get_mut<Force>().map() += force_b;
}

inline void apply_spring_elastic_force(flecs::iter& it, size_t, Spring& spring) {
    apply_spring_force(spring, spring.k_s, Real(0));
}

inline void apply_spring_damping_force(flecs::iter& it, size_t, Spring& spring) {
    apply_spring_force(spring, Real(0), spring.k_d);
}

inline void integrate_position(flecs::iter& it, size_t, Position& x, const Velocity& v) {
    physics::integration::update_position(x, v, it.delta_time());
}

inline void integrate_velocity(flecs::iter& it, size_t, Velocity& v, const Force& f, const Mass& m) {
    v.map() += it.delta_time() * (f.map() / m);
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
    components::register_render_components(ecs);
    queries::seed(ecs);

    // 2) Runtime data
    props::seed(ecs);
    state::seed(ecs);

    // hint: override core props here
    props::dt.set<Real>(Real(1.0f / 60.0f));
    props::gravity.set<vec3f>({0.0f, -9.81f, 0.0f});
    props::paused.set<bool>(false);

    // non-GL singletons
    ecs.set<MousePickState>({});

    // 3) Services
    graphics::init(ecs, {800, 600, "Base Simulator"});

    // GL singletons (after graphics::init)
    ecs.set<SpringRenderer>({});
    ecs.set<ParticleRenderer>({});

    // 4) Systems
    systems::install_scene_systems(ecs);
    systems::install_render_systems(ecs);

    ecs.observer<Particle>()
        .event(flecs::OnAdd)
        .each([](flecs::entity e, const Particle&) {
            e.add<Force>();
        });

    // mark scene dirty when particles are added/removed
    ecs.observer<Particle>()
        .event(flecs::OnAdd)
        .event(flecs::OnRemove)
        .run([](flecs::iter& it) {
            state::dirty.get_mut<bool>() = true;
        });

    // =========================================================================
    // System handles
    // =========================================================================

    flecs::system clear_force;
    flecs::system apply_gravity;
    flecs::system apply_spring_elastic_force;
    flecs::system apply_spring_damping_force;
    flecs::system integrate_position;
    flecs::system integrate_velocity;

    auto algorithm = ecs.system("Explicit Euler")
        .kind(flecs::PreUpdate)
        .run([&](flecs::iter& it) {
            const auto& dt = props::dt.get<Real>();
            if (clear_force.enabled()) clear_force.run(dt);
            if (apply_gravity.enabled()) apply_gravity.run(dt);
            if (apply_spring_elastic_force.enabled()) apply_spring_elastic_force.run(dt);
            if (apply_spring_damping_force.enabled()) apply_spring_damping_force.run(dt);
            if (integrate_position.enabled()) integrate_position.run(dt);
            if (integrate_velocity.enabled()) integrate_velocity.run(dt);
        });

    // =========================================================================
    // Flow block (order = flow)
    // =========================================================================

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
                const auto& gravity = props::gravity.get<vec3f>();
                f.map() += m * gravity.map();
            });

        apply_spring_elastic_force = ecs.system<Spring>("Apply Spring Elastic Force")
            .kind(0)
            .each(flow::apply_spring_elastic_force);

        apply_spring_damping_force = ecs.system<Spring>("Apply Spring Damping Force")
            .kind(0)
            .each(flow::apply_spring_damping_force);

        integrate_position = ecs.system<Position, const Velocity>("Integrate Position")
            .with<Particle>()
            .without<IsPinned>()
            .kind(0)
            .each(flow::integrate_position);

        integrate_velocity = ecs.system<Velocity, const Force, const Mass>("Integrate Velocity")
            .with<Particle>()
            .without<IsPinned>()
            .kind(0)
            .each(flow::integrate_velocity);
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
