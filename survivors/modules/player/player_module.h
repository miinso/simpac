#pragma once

#include <modules/base_module.h>

namespace player {
    struct PlayerModule : public BaseModule<PlayerModule> {
    public:
        PlayerModule(flecs::world world) : BaseModule(world){};

    private:
        void register_components(flecs::world world);
        void register_systems(flecs::world world);

        friend struct BaseModule<PlayerModule>;
    };
} // namespace player
