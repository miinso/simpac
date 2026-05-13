#pragma once

#include "flecs.h"
#include "graphics.h"

#include "../components.h"
#include "../sim/bridge.h"
#include "../queries.h"
#include "../vars.h"

#include <cstdio>

// =========================================================================
// sim globals (populated by cloth::core module on import)
// =========================================================================

namespace sim {

inline flecs::entity SimulateBegin;
inline flecs::entity Simulate;
inline flecs::entity SimulateEnd;

inline physics::Model model;
inline physics::State state_0;
inline physics::Bridge bridge;
inline bool model_dirty = true;
inline Eigen::Vector3r gravity = Eigen::Vector3r::Zero();

} // namespace sim

// =========================================================================
// cloth::core module
// =========================================================================

namespace cloth {

struct core {
    core(flecs::world& ecs) {
        components::register_particle_components(ecs);
        components::register_element_components(ecs);
        components::register_constraint_components(ecs);
        components::register_solver_stats(ecs);
        queries::seed(ecs);
        props::seed(ecs);
        state::seed(ecs);
        props::dt.set<Real>(Real(1.0f / 60.0f));
        props::gravity.set<vec3f>({0.0f, -9.81f, 0.0f});
        props::paused.set<bool>(false);
        ecs.ensure<Solver>();

        // -- sub-phases: SimulateBegin → Simulate → SimulateEnd --
        sim::SimulateBegin = ecs.entity("sim::SimulateBegin")
            .add(flecs::Phase)
            .depends_on(flecs::PreUpdate);
        sim::Simulate = ecs.entity("sim::Simulate")
            .add(flecs::Phase)
            .depends_on(sim::SimulateBegin);
        sim::SimulateEnd = ecs.entity("sim::SimulateEnd")
            .add(flecs::Phase)
            .depends_on(sim::Simulate);

        // -- observers --
        ecs.observer<Particle>()
            .event(flecs::OnAdd)
            .each([](flecs::entity e, const Particle&) { e.add<ParticleState>(); });

        ecs.observer<Particle>()
            .event(flecs::OnAdd)
            .event(flecs::OnRemove)
            .run([](flecs::iter&) { sim::model_dirty = true; });

        ecs.observer<Spring>()
            .event(flecs::OnSet)
            .event(flecs::OnAdd)
            .event(flecs::OnRemove)
            .each([](flecs::iter&, size_t, Spring&) { sim::model_dirty = true; });

        ecs.observer<IsPinned>()
            .event(flecs::OnAdd)
            .event(flecs::OnRemove)
            .run([](flecs::iter&) { sim::model_dirty = true; });

        // -- bridge systems --
        ecs.system("Rebuild")
            .kind(sim::SimulateBegin)
            .run([](flecs::iter& it) {
                if (!sim::model_dirty) return;
                auto world = it.world();
                sim::model = sim::bridge.build(world);
                sim::state_0 = sim::model.state();
                sim::model_dirty = false;
                printf("[Solver] rebuilt: %d particles, %d springs\n",
                       sim::model.particle_count, sim::model.spring_count);
            });

        ecs.system("Gather")
            .kind(sim::SimulateBegin)
            .run([](flecs::iter&) {
                sim::bridge.gather(sim::state_0);
            });

        ecs.system("Scatter")
            .kind(sim::SimulateEnd)
            .run([](flecs::iter&) {
                sim::bridge.scatter(sim::state_0);
            });

        // -- scene time --
        ecs.system("Scene::UpdateWallTime")
            .kind(flecs::PreUpdate)
            .run([](flecs::iter& it) {
                auto& wall_time = state::wall_time.get_mut<Real>();
                wall_time += it.delta_time();
            });

        ecs.system("Scene::UpdateSimTime")
            .kind(flecs::PreUpdate)
            .run([](flecs::iter& it) {
                if (props::paused.get<bool>()) return;
                const auto& dt = props::dt.get<Real>();
                auto& sim_time = state::sim_time.get_mut<Real>();
                auto& frame_count = state::frame_count.get_mut<int>();
                sim_time += dt;
                frame_count += 1;
            });
    }
};

} // namespace cloth

// =========================================================================
// scene loading (free function -- not initialization, just data)
// =========================================================================

namespace sim {

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
