#include "core_module.h"
#include "body.h"
#include "components.h"
#include "geometry.h"


namespace core {
    void CoreModule::register_components(flecs::world &world) {
        world.component<particle_q>();
        world.component<particle_qd>();
        world.component<particle_f>();
        world.component<Settings>().add(flecs::Singleton);
        // world.component<PARTICLE_FLAG_ACTIVE>();
        // world.component<PARTICLE_FLAG_STATIC>();

        geometry::shape s;
        body::body b;

    }

    void CoreModule::register_systems(flecs::world &world) {}
} // namespace core
