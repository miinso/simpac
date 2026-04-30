#pragma once

// shared simulation infrastructure -- solver files only need algorithm-specific code

#include "flecs.h"
#include "graphics.h"

#include "components.h"
#include "sim/bridge.h"
#include "queries.h"
#include "vars.h"
#include "systems.h"
#include <cstdio>
#include <string>

struct InitConfig {
    int width = 800;
    int height = 600;
    const char* title = "Simulator";
};

namespace sim {

inline flecs::entity Simulate;
inline physics::Model model;
inline physics::State state_0;
inline physics::Bridge bridge;
inline bool model_dirty = true;
inline Eigen::Vector3r gravity = Eigen::Vector3r::Zero();

inline void seed_phases(flecs::world& ecs) {
    Simulate = ecs.entity("sim::Simulate")
        .add(flecs::Phase)
        .depends_on(flecs::PreUpdate);
}

// generic rebuild -- no solver-specific buffer resets
inline void rebuild(flecs::iter& it) {
    if (!model_dirty) return;
    auto world = it.world();
    model = bridge.build(world);
    state_0 = model.state();
    model_dirty = false;
    printf("[Solver] rebuilt: %d particles, %d springs\n",
           model.particle_count, model.spring_count);
}

inline void gather(flecs::iter&) {
    bridge.gather(state_0);
}

inline void scatter(flecs::iter&) {
    bridge.scatter(state_0);
}

inline void init(flecs::world& ecs, InitConfig cfg) {
    components::register_particle_components(ecs);
    components::register_constraint_components(ecs);
    components::register_render_components(ecs);
    components::register_solver_stats(ecs);

    queries::seed(ecs);
    props::seed(ecs);
    state::seed(ecs);
    seed_phases(ecs);

    props::dt.set<Real>(Real(1.0f / 60.0f));
    props::gravity.set<vec3f>({0.0f, -9.81f, 0.0f});
    props::paused.set<bool>(false);

    ecs.ensure<Solver>();
    ecs.set<ParticleInteractionState>({});
    ecs.set<TriangleInteractionState>({});

    graphics::init(ecs, {cfg.width, cfg.height, cfg.title});
    ecs.set<SpringRenderer>({});
    ecs.set<ParticleRenderer>({});
    ecs.set<TriangleRenderer>({});

    systems::install_scene_systems(ecs);
    systems::install_render_systems(ecs);
}

inline void install(flecs::world& ecs) {
    ecs.observer<Particle>()
        .event(flecs::OnAdd)
        .each([](flecs::entity e, const Particle&) { e.add<ParticleState>(); });

    ecs.observer<Particle>()
        .event(flecs::OnAdd)
        .event(flecs::OnRemove)
        .run([](flecs::iter&) { model_dirty = true; });

    ecs.observer<Spring>()
        .event(flecs::OnSet)
        .event(flecs::OnAdd)
        .event(flecs::OnRemove)
        .each([](flecs::iter&, size_t, Spring&) { model_dirty = true; });

    // pin toggles flow through bridge rebuild so b/ solvers see the new inv_mass
    ecs.observer<IsPinned>()
        .event(flecs::OnAdd)
        .event(flecs::OnRemove)
        .run([](flecs::iter&) { model_dirty = true; });

    ecs.system("Rebuild")
        .kind(Simulate)
        .run(rebuild);

    ecs.system("Gather")
        .kind(Simulate)
        .run(gather);
}

// scatter must register AFTER integrator for pipeline ordering
inline void install_scatter(flecs::world& ecs) {
    ecs.system("Scatter")
        .kind(Simulate)
        .run(scatter);
}

inline void load_scene(flecs::world& ecs, const char* path) {
    const std::string scene_path = graphics::npath(path);
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
}

} // namespace sim
