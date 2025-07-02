#pragma once

#include <flecs.h>

#include <modules/engine/rendering/components.h>

#include "components.h"
#include "physics_module.h"

namespace physics {
    namespace systems {
        inline void reset_desired_velocity(const Velocity &vel, DesiredVelocity &desired_vel) {
            desired_vel.value = vel.value;
        };

        inline void integrate_velocity(flecs::iter &it, size_t, Velocity &vel,
                                       const DesiredVelocity &desired_vel,
                                       const AccelerationSpeed &acc) {
            float t = std::min(PHYSICS_TICK_LENGTH, it.delta_system_time()) * acc.value;
            vel.value = vel.value + t * (desired_vel.value - vel.value);
        };

        inline void integrate_position(flecs::iter &it, size_t, core::Position &pos,
                                       const Velocity &vel) {
            pos.value += vel.value * std::min(PHYSICS_TICK_LENGTH, it.delta_system_time());
        };

        inline void detect_collision(flecs::entity body_a, CollisionRecordList &list,
                                     const core::Position &x_a, const Collider &collider_a) {
            flecs::world staged_world = body_a.world(); // this makes a "staged" world (not sure
                                                        // what it means. a freezed copy of the
                                                        // world? idk there's no docs for it)

            auto query_all_visibles =
                    staged_world.query_builder<const core::Position, const Collider>()
                            .with<rendering::Visible>()
                            .build();

            query_all_visibles.each([&](flecs::entity body_b, const core::Position &x_b,
                                        const Collider &collider_b) {
                if (body_a.id() > body_b.id())
                    return;
                if ((collider_a.collision_filter & collider_b.collision_type) == none)
                    return;

                float radius = collider_a.radius + collider_b.radius;
                float distance_sqr = (x_a.value - x_b.value).squaredNorm();
                if (distance_sqr < radius * radius) {
                    // collision between different types is treated `significant`
                    if ((collider_a.collision_type & collider_b.collision_type) == none) {
                        // these are for attack/damage/interaction logic
                        // will be consumed in `CollidedWith` creation
                        list.significant_collisions.push_back({body_a, body_b});
                    }

                    // correct_position? is this for static/dynamic body?
                    if (collider_a.correct_position && collider_b.correct_position) {
                        // these go to collision resolution
                        list.records.push_back({body_a, body_b});
                    }
                }
            });
        };

        inline void create_collision_relationship(CollisionRecordList &list) {
            for (auto &record: list.significant_collisions) {
                auto a = record.a;
                auto b = record.b;

                // TODO: see how the nonfragment feature turns out
                a.add<CollidedWith>(b);
                b.add<CollidedWith>(a);
            }

            list.significant_collisions.clear();
        }

        inline void resolve_collision(CollisionRecordList &list) {
            for (auto &record: list.records) {
                auto a = record.a;
                auto b = record.b;

                auto x_a = a.try_get<core::Position>()->value;
                auto x_b = b.try_get<core::Position>()->value;
                auto v_a = a.try_get<Velocity>()->value;
                auto v_b = b.try_get<Velocity>()->value;

                float radius = a.try_get<Collider>()->radius + b.try_get<Collider>()->radius;

                auto delta = x_a - x_b;
                auto direction = delta.normalized();

                auto grad_a = direction;
                auto grad_b = -direction;

                auto violation = radius - delta.norm();

                auto correction_a = -grad_a * violation * 0.5f;
                auto correction_b = -grad_b * violation * 0.5f;

                v_a += correction_a;
                v_b += correction_b;

                x_a += correction_a / 2;
                x_b += correction_b / 2;
            }

            list.records.clear();
        }

        inline void delete_collision_relationship(flecs::iter &it, size_t i) {
            auto a = it.entity(i);
            auto b = it.pair(0).second();

            a.remove<CollidedWith>(b);
        }

    } // namespace systems
} // namespace physics
