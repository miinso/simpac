#pragma once

#include <flecs.h>

#include <modules/engine/core/components.h>
#include <modules/engine/physics/components.h>

namespace physics::queries {
    inline flecs::query<core::Position, Collider> visible_collision_bodies_query;
    inline flecs::query<core::Position, Collider> box_collider_query;
} // namespace physics::queries
