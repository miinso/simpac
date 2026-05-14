#pragma once

#include "../element/element.h"
#include "../sim/bridge.h"

#include "flecs.h"
#include "tags.h"
#include "graphics.h"

#include <cstdio>
#include <deque>
#include <string>

// =========================================================================
// sim globals (populated by cloth::bridge module on import)
// =========================================================================

namespace sim {

inline flecs::entity SimulateBegin;
inline flecs::entity Simulate;
inline flecs::entity SimulateEnd;

inline physics::Model model;
inline physics::State state_0;
inline physics::Bridge bridge;
inline Eigen::Vector3r gravity = Eigen::Vector3r::Zero();

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

// =========================================================================
// props / state / stats
// =========================================================================

namespace props {
inline flecs::entity dt;
inline flecs::entity gravity;
inline flecs::entity paused;
inline flecs::entity substeps;
inline flecs::entity default_dihedral_bending_stiffness;

inline void seed(flecs::world& ecs) {
    dt = ecs.entity("::Config::Scene::dt")
        .set<Real>(Real(1.0f / 60.0f))
        .add<Configurable>();
    gravity = ecs.entity("::Config::Scene::gravity")
        .set<vec3r>({0.0f, -9.81f, 0.0f})
        .add<Configurable>();
    paused = ecs.entity("::Config::Scene::paused")
        .set<bool>(false)
        .add<Configurable>();
    substeps = ecs.entity("::Config::Scene::substeps")
        .set<int>(1)
        .add<Configurable>();
    default_dihedral_bending_stiffness = ecs.entity("::Config::Scene::default_dihedral_bending_stiffness")
        .set<Real>(Real(1.0))
        .add<Configurable>();
}
} // namespace props

namespace state {
inline flecs::entity wall_time;
inline flecs::entity sim_time;
inline flecs::entity frame_count;
inline flecs::entity dirty;

inline void seed(flecs::world& ecs) {
    wall_time = ecs.entity("::Scene::wall_time").set<Real>(Real(0.0f));
    sim_time = ecs.entity("::Scene::sim_time").set<Real>(Real(0.0f));
    frame_count = ecs.entity("::Scene::frame_count").set<int>(0);
    dirty = ecs.entity("::Scene::dirty").set<bool>(false);
}
} // namespace state

struct Solver {
    bool exploded = false;
    int cg_iterations = 0;
    Real cg_error = 0;
    std::deque<std::string> cg_history;
    int cg_history_max_lines = 15;
};

namespace queries {
inline flecs::query<const Position> particle_query;
inline flecs::query<const Spring> spring_query;
inline flecs::query<const Triangle> triangle_query;
inline flecs::query<const Position, const Mass, ParticleState> particle_pick_query;

inline void seed(flecs::world& ecs) {
    particle_query = ecs.query_builder<const Position>()
        .with<Particle>().cached().build();
    spring_query = ecs.query_builder<const Spring>()
        .cached().build();
    triangle_query = ecs.query_builder<const Triangle>()
        .cached().build();
    particle_pick_query = ecs.query_builder<const Position, const Mass, ParticleState>()
        .with<Particle>().cached().build();
}

inline int num_particles() { return (int)particle_query.count(); }
inline int num_springs() { return (int)spring_query.count(); }
} // namespace queries

// =========================================================================
// cloth::bridge module
// =========================================================================

namespace cloth {

struct bridge {
    bridge(flecs::world& ecs) {
        ecs.component<Solver>().add(flecs::Singleton);
        queries::seed(ecs);
        props::seed(ecs);
        state::seed(ecs);
        props::dt.set<Real>(Real(1.0f / 60.0f));
        props::gravity.set<vec3r>({0.0f, -9.81f, 0.0f});
        props::paused.set<bool>(false);
        ecs.ensure<Solver>();

        sim::SimulateBegin = ecs.entity("sim::SimulateBegin")
            .add(flecs::Phase)
            .depends_on(flecs::PreUpdate);
        sim::Simulate = ecs.entity("sim::Simulate")
            .add(flecs::Phase)
            .depends_on(sim::SimulateBegin);
        sim::SimulateEnd = ecs.entity("sim::SimulateEnd")
            .add(flecs::Phase)
            .depends_on(sim::Simulate);

        ecs.system("Rebuild")
            .kind(sim::SimulateBegin)
            .run([](flecs::iter& it) {
                if (!sim::model_dirty) return;
                auto world = it.world();
                sim::bridge.default_bending_stiffness = props::default_dihedral_bending_stiffness.get<Real>();
                sim::model = sim::bridge.build(world);
                sim::state_0 = sim::model.state();
                sim::model_dirty = false;
                printf("[Solver] rebuilt: %d particles, %d springs, %d hinges (k_bend=%.4f)\n",
                       sim::model.particle_count, sim::model.spring_count,
                       sim::model.edge_count,
                       (float)props::default_dihedral_bending_stiffness.get<Real>());
            });

        ecs.system("Gather")
            .kind(sim::SimulateBegin)
            .run([](flecs::iter&) {
                sim::bridge.gather(sim::state_0);
            });


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
