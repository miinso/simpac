#pragma once

#include <flecs.h>

#include <modules/engine/core/components.h>
#include "components.h"

namespace physics {
    namespace queries {
        extern flecs::query<core::Position, Collider> visible_collision_bodies_query;
        extern flecs::query<core::Position, Collider> box_collider_query;
    } // namespace queries

    // struct queries {
    //     static flecs::query<core::Position, Collider> visible_collision_bodies_query;
    //     static flecs::query<core::Position, Collider> box_collider_query;
    // };
} // namespace physics
