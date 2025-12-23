#pragma once

#include <flecs.h>
#include <raylib.h>
#include <raymath.h>

#include <modules/base_module.h>
#include <modules/engine/core/components.h>

#include "components.h"

namespace physics {
    constexpr float PHYSICS_TICK_LENGTH = 0.016;

    // meaning, this one interacts with `enemy` and `env`
    static CollisionFilter player_filter = static_cast<CollisionFilter>(enemy | environment);
    static CollisionFilter enemy_filter =
            static_cast<CollisionFilter>(player | enemy | environment);
    static CollisionFilter environment_filter = static_cast<CollisionFilter>(player | enemy);

    struct PhysicsModule : public BaseModule<PhysicsModule> {
        friend struct BaseModule<PhysicsModule>;

    public:
        PhysicsModule(flecs::world &world) : BaseModule(world){};

    private:
        flecs::entity m_physics_tick;

        void register_components(flecs::world &world);
        void register_systems(flecs::world &world);
        void register_pipeline(flecs::world &world);
        void register_queries(flecs::world &world);
    };
} // namespace physics
