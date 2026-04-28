#pragma once

#include "flecs.h"
#include "tags.h"
#include "components.h"

#include <deque>
#include <string>

namespace props {
inline flecs::entity dt;
inline flecs::entity gravity;
inline flecs::entity paused;

inline void seed(flecs::world& ecs) {
    dt = ecs.entity("Config::Scene::dt")
        .set<Real>(Real(1.0f / 60.0f))
        .add<Configurable>();
    gravity = ecs.entity("Config::Scene::gravity")
        .set<vec3r>({0.0f, -9.81f, 0.0f})
        .add<Configurable>();
    paused = ecs.entity("Config::Scene::paused")
        .set<bool>(false)
        .add<Configurable>();
}
} // namespace props

namespace state {
inline flecs::entity wall_time;
inline flecs::entity sim_time;
inline flecs::entity frame_count;
inline flecs::entity dirty;

inline void seed(flecs::world& ecs) {
    wall_time = ecs.entity("Scene::wall_time").set<Real>(Real(0.0f));
    sim_time = ecs.entity("Scene::sim_time").set<Real>(Real(0.0f));
    frame_count = ecs.entity("Scene::frame_count").set<int>(0);
    dirty = ecs.entity("Scene::dirty").set<bool>(false);
}
} // namespace state

// UI stats singleton -- solver working buffers live in main_*.cpp
struct Solver {
    bool exploded = false;
    int cg_iterations = 0;
    Real cg_error = 0;
    std::deque<std::string> cg_history;
    int cg_history_max_lines = 15;
};

namespace components {
inline void register_solver_stats(flecs::world& ecs) {
    ecs.component<Solver>().add(flecs::Singleton);
}
} // namespace components
