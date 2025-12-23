#include "core_module.h"
#include "components.h"

namespace core {
    void CoreModule::register_components(flecs::world &world) {
        // world.component<particle_q>();
        // world.component<particle_qd>();
        // world.component<particle_f>();
        // world.component<PARTICLE_FLAG_ACTIVE>();
        // world.component<PARTICLE_FLAG_STATIC>();
    }

    void CoreModule::register_systems(flecs::world &world) {}
} // namespace core
