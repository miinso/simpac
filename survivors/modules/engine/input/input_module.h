#pragma once

#include <modules/base_module.h>

namespace input {
    struct InputModule : public BaseModule<InputModule> {
    public:
        InputModule(flecs::world &world) : BaseModule(world) {}

    private:
        void register_components(flecs::world &world);
        void register_systems(flecs::world &world);

        friend struct BaseModule<InputModule>;
    };
} // namespace input
