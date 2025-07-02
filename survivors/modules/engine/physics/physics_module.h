#pragma once

#include <flecs.h>

#include <modules/base_module.h>
#include <modules/engine/core/components.h>

#include "components.h"

namespace physics {
    constexpr float PHYSICS_TICK_LENGTH = 0.016;
    static CollisionFilter player_filter = static_cast<CollisionFilter>(player | enemy);
    static CollisionFilter enemy_filter = static_cast<CollisionFilter>(player | enemy);

    struct PhysicsModule : public BaseModule<PhysicsModule> {
        friend struct BaseModule<PhysicsModule>;

    public:
        PhysicsModule(flecs::world &world) : BaseModule(world){};

        static flecs::entity m_physicsTick;

    private:
        void register_components(flecs::world &world);
        void register_systems(flecs::world &world);
        void register_pipeline(flecs::world &world);

        // void register_queries(flecs::world &world);

        // static bool collide(flecs::entity &self, flecs::entity &other) {
        //     auto x_self = self.try_get<core::Position>()->value;
        //     auto x_other = other.try_get<core::Position>()->value;

        //     float combined_radius = self.try_get<physics::Collider>()->radius +
        //                             other.try_get<physics::Collider>()->radius;
        //     auto delta = x_other - x_self;
        //     auto distance_sqr = delta.squaredNorm();

        //     return distance_sqr < combined_radius * combined_radius;
        // };
    };
} // namespace physics
