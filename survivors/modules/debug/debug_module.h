#pragma once

#include <modules/base_module.h>

namespace debug {
    struct DebugModule : public BaseModule<DebugModule> {
    public:
        DebugModule(flecs::world world) : BaseModule(world){};

    private:
        flecs::system debug_collidable_entity_id;
        flecs::system debug_collider_bound;
        flecs::system debug_circle_collider;
        flecs::system debug_box_collider;
        flecs::system debug_FPS;
        flecs::system debug_entity_count;
        flecs::system debug_mouse_pos;
        flecs::system debug_grid;
        flecs::system debug_closest_enemy;

        void register_components(flecs::world &world);
        void register_systems(flecs::world &world);
        void register_entities(flecs::world &world);

        friend struct BaseModule<DebugModule>;
    };
} // namespace debug
