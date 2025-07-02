#pragma once

#include <modules/base_module.h>

namespace ai {
    struct AIModule : public BaseModule<AIModule> {
    public:
        AIModule(flecs::world world) : BaseModule(world){};

    private:
        void register_components(flecs::world world);
        void register_systems(flecs::world world);

        friend struct BaseModule<AIModule>;
    };
} // namespace ai
