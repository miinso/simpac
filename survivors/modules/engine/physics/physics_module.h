#pragma once

#include <flecs.h>
#include <raylib.h>
#include <raymath.h>

#include <modules/base_module.h>
#include <modules/engine/core/components.h>


#include "components.h"

namespace physics {
    constexpr float PHYSICS_TICK_LENGTH = 0.016;
    static CollisionFilter player_filter = static_cast<CollisionFilter>(player | enemy);
    static CollisionFilter enemy_filter = static_cast<CollisionFilter>(player | enemy);
    static CollisionFilter environment_filter = static_cast<CollisionFilter>(player | enemy);

    struct PhysicsModule : public BaseModule<PhysicsModule> {
        friend struct BaseModule<PhysicsModule>;

    public:
        PhysicsModule(flecs::world &world) : BaseModule(world){};


        static Vector2 collide_circle_circle(const CircleCollider *a, const core::Position *a_pos,
                                       CollisionInfo &a_info, const CircleCollider *b,
                                       const core::Position *b_pos, CollisionInfo &b_info) {
            float combinedRadius = a->radius + b->radius;

            // Find the distance and adjust to resolve the overlap
            auto delta = (b_pos->value - a_pos->value);
            auto direction = delta.normalized();
            // Vector2 moveDirection = Vector2Normalize(direction);
            // float overlap = combinedRadius - Vector2Length(direction);
            float overlap = combinedRadius - delta.norm();

            a_info.normal = Vector2Negate({direction.x(), direction.y()});
            b_info.normal = {direction.x(), direction.y()};

            auto correction = direction * overlap;
            return {correction.x(), correction.y()};
        }

        static Vector2 collide_circle_rec(const CircleCollider *a, core::Position *a_pos,
                                          CollisionInfo &a_info, const Collider *b,
                                          core::Position *b_pos, CollisionInfo &b_info) {
            float recCenterX = b_pos->value.x() + b->bounds.x + b->bounds.width / 2.0f;
            float recCenterY = b_pos->value.y() + b->bounds.y + b->bounds.height / 2.0f;

            float halfWidth = b->bounds.width / 2.0f;
            float halfHeight = b->bounds.height / 2.0f;

            float dx = a_pos->value.x() - recCenterX;
            float dy = a_pos->value.y() - recCenterY;

            float absDx = fabsf(dx);
            float absDy = fabsf(dy);

            Vector2 overlap = {0, 0};

            if (absDx > (halfWidth + a->radius))
                return overlap;
            if (absDy > (halfHeight + a->radius))
                return overlap;

            if (absDx <= halfWidth || absDy <= halfHeight) {
                // Side collision — resolve with axis-aligned MTV
                float overlapX = (halfWidth + a->radius) - absDx;
                float overlapY = (halfHeight + a->radius) - absDy;


                if (overlapX < overlapY) {
                    overlap.x = dx < 0 ? overlapX : -overlapX;
                } else {
                    overlap.y = dy < 0 ? overlapY : -overlapY;
                }
                a_info.normal = Vector2Normalize(Vector2Negate(overlap));
                b_info.normal = Vector2Normalize(overlap);
                return overlap;
            }

            // Corner collision
            float cornerDx = absDx - halfWidth;
            float cornerDy = absDy - halfHeight;

            float cornerDistSq = cornerDx * cornerDx + cornerDy * cornerDy;
            float radius = a->radius;

            if (cornerDistSq < radius * radius) {
                // std::cout << "collided 2" << std::endl;
                float dist = sqrtf(cornerDistSq);

                if (dist == 0.0f)
                    dist = 0.01f; // Avoid divide by zero

                float overlap_length = radius - dist;
                float nx = cornerDx / dist;
                float ny = cornerDy / dist;

                overlap = {nx * overlap_length * ((dx < 0) ? 1.0f : -1.0f),
                           ny * overlap_length * ((dy < 0) ? 1.0f : -1.0f)};

                a_info.normal = Vector2Normalize(Vector2Negate(overlap));
                b_info.normal = Vector2Normalize(overlap);
            }

            return overlap;
        }

    private:
        flecs::entity m_physicsTick;

        void register_components(flecs::world &world);
        void register_systems(flecs::world &world);
        void register_pipeline(flecs::world &world);
        void register_queries(flecs::world &world);

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
