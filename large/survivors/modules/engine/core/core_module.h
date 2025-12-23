#pragma once

#include <modules/base_module.h>

#include "components.h"

namespace core {
    struct CoreModule : public BaseModule<CoreModule> {
        friend struct BaseModule<CoreModule>;

    public:
        CoreModule(flecs::world &world) : BaseModule(world) {}
        GameSettings *game_settings;

    private:
        void register_components(flecs::world &world);
        void register_systems(flecs::world &world);
        void register_queries(flecs::world &world);
    };
} // namespace core
