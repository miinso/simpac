#pragma once

#include <modules/base_module.h>

namespace core {
    struct CoreModule : public BaseModule<CoreModule> {
        friend struct BaseModule<CoreModule>;

    public:
        CoreModule(flecs::world &world) : BaseModule(world) {}

    private:
        void register_components(flecs::world &world);
        void register_systems(flecs::world &world);
    };
} // namespace core
