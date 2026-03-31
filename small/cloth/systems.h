#pragma once

// convenience header - includes all system implementations
#include "vars.h"
#include "queries.h"
#include "systems/render/module.h"

// verb convention:
// register_* : register types/components/meta only.
// seed*      : create runtime entities and default values.
// install_*  : define/install systems.

namespace systems {

inline void install_scene_systems(flecs::world& ecs) {
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

} // namespace systems

// TODO: put application/engine-specific ecs setup and glue code here

