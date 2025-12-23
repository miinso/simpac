#include "tilemap_module.h"

#include "systems.h"

namespace tilemap {
    void TilemapModule::register_components(flecs::world &world) {}

    void TilemapModule::register_systems(flecs::world &world) {
        world.system<const Tilemap>("Init tilemap")
                .kind(flecs::OnStart)
                .each(systems::create_tilemap);
    }
} // namespace tilemap
