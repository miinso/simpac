#pragma once

#include <modules/base_module.h>

namespace tilemap {
    struct TilemapModule : public BaseModule<TilemapModule> {
    public:
        TilemapModule(flecs::world world) : BaseModule(world) {}

    private:
        void register_components(flecs::world &world);
        void register_systems(flecs::world &world);

        friend struct BaseModule<TilemapModule>;
    };
} // namespace tilemap
