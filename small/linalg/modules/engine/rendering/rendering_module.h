#pragma once

#include <modules/base_module.h>

namespace rendering {
    struct RenderingModule : public BaseModule<RenderingModule> {
    public:
        explicit RenderingModule(flecs::world &world) : BaseModule(world) {}

    private:
        void register_components(flecs::world &world);
        void register_systems(flecs::world &world);
        void register_pipeline(flecs::world &world);
        void register_submodules(flecs::world &world);
        void register_queries(flecs::world &world);

        friend struct BaseModule<RenderingModule>;
    };
} // namespace rendering
