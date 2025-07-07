#pragma once

#include <flecs.h>

#include <modules/engine/rendering/components.h>

#include "components.h"
#include "physics_module.h"

namespace physics {
    namespace systems {
        ///////////////////// wow
        /**
         * Correct the positions of the entity (if they are not static nor "triggers") by the
         * overlap amount
         * @param a entity 1
         * @param a_col entity 1 mutable position
         * @param b entity 2
         * @param b_col entity 2 mutable position
         * @param overlap amount of overlap of the two entities
         */
        inline void correct_positions(flecs::entity &a, const Collider *a_col,
                                      CollisionInfo &a_info, flecs::entity &b,
                                      const Collider *b_col, CollisionInfo &b_info,
                                      Vector2 overlap) {
            core::Position *a_pos = a.try_get_mut<core::Position>();
            core::Position *b_pos = b.try_get_mut<core::Position>();

            float a_move_ratio = 0.5f;
            float b_move_ratio = 0.5f;

            if (a_col->static_body) {
                a_move_ratio = 0;
                b_move_ratio = 1.0f;
            }
            if (b_col->static_body) {
                a_move_ratio = 1.0f;
                b_move_ratio = 0;
            }

            if ((!a_col->correct_position && !a_col->static_body) ||
                (!b_col->correct_position && !b_col->static_body)) {
                a_move_ratio = 0.0f;
                b_move_ratio = 0.0f;
            }
            auto correction = Eigen::Vector3f(overlap.x, overlap.y, 0);
            a_pos->value = a_pos->value - correction * a_move_ratio * 0.9;
            b_pos->value = b_pos->value + correction * b_move_ratio * 0.9;
        }

        /**
         * Resolve Circle vs Circle collision
         * @param a circle 1 entity
         * @param base_col circle 1 collider
         * @param b circle 2 entity
         * @param other_base_col circle 2 entity
         * @return if the circles collided
         */
        inline bool handle_circle_circle(flecs::entity &a, const Collider *a_col,
                                         CollisionInfo &a_info, flecs::entity &b,
                                         const Collider *b_col, CollisionInfo &b_info) {
            auto a_pos = a.try_get<core::Position>()->value;
            auto b_pos = b.try_get<core::Position>()->value;

            const CircleCollider *col = a.try_get<CircleCollider>();
            const CircleCollider *other_col = b.try_get<CircleCollider>();

            if (!CheckCollisionCircles({a_pos.x(), a_pos.y()}, col->radius, {b_pos.x(), b_pos.y()},
                                       other_col->radius)) {
                return false;
            }

            Vector2 overlap =
                    PhysicsModule::collide_circles(col, a.try_get<core::Position>(), a_info,
                                                   other_col, b.try_get<core::Position>(), b_info);

            correct_positions(a, a_col, a_info, b, b_col, b_info, overlap);

            Vector2 contact_point = Vector2{a_pos.x(), a_pos.y()} + b_info.normal * col->radius;
            a_info.contact_point = contact_point;
            b_info.contact_point = contact_point;

            return true;
        }

        /**
         * Resolve circle vs rectangle collision
         * @param a circle entity
         * @param a_col circle base collider
         * @param b box entity
         * @param b_col box base collider
         * @return if there was a collision
         */
        inline bool handle_circle_rec_collision(flecs::entity &a, const Collider *a_col,
                                                CollisionInfo &a_info, flecs::entity &b,
                                                const Collider *b_col, CollisionInfo &b_info) {
            auto a_pos = a.try_get<core::Position>()->value;
            auto b_pos = b.try_get<core::Position>()->value;
            const CircleCollider *circle_col = a.try_get<CircleCollider>();
            if (!CheckCollisionCircleRec({a_pos.x(), a_pos.y()}, circle_col->radius,
                                         {b_pos.x() + b_col->bounds.x, b_pos.y() + b_col->bounds.y,
                                          b_col->bounds.width, b_col->bounds.height})) {
                return false;
            }

            Vector2 overlap = PhysicsModule::collide_circle_rec(
                    circle_col, a.try_get_mut<core::Position>(), a_info, b_col,
                    b.try_get_mut<core::Position>(), b_info);

            correct_positions(a, a_col, a_info, b, b_col, b_info, overlap);

            Vector2 contact_point =
                    Vector2{a_pos.x(), a_pos.y()} + b_info.normal * circle_col->radius;
            a_info.contact_point = contact_point;
            b_info.contact_point = contact_point;

            return true;
        }

        /**
         * Resolve rectangle vs rectangle collision
         * @param a box 1 entity
         * @param a_col box 1 collider
         * @param b box 2 entity
         * @param b_col box 2 collider
         * @return if the boxes collided or not
         */
        inline bool handle_boxes_collision(flecs::entity &a, const Collider *a_col,
                                           flecs::entity &b, const Collider *b_col) {
            auto a_pos = a.try_get<core::Position>()->value;
            auto b_pos = b.try_get<core::Position>()->value;
            if (!CheckCollisionRecs({a_pos.x() + a_col->bounds.x, a_pos.y() + a_col->bounds.y,
                                     a_col->bounds.width, a_col->bounds.height},
                                    {b_pos.x() + b_col->bounds.x, b_pos.y() + b_col->bounds.y,
                                     b_col->bounds.width, b_col->bounds.height})) {
                return false;
            }

            // TODO need to figure overlap for boxes and update the positions properly
            return true;
        }

        using CollisionHandler =
                std::function<bool(flecs::entity &, const Collider *, CollisionInfo &,
                                   flecs::entity &, const Collider *, CollisionInfo &)>;

        /**
         * Map for collision type handling, we point to the correct position in the array with the
         * Collider type enum
         */
        inline CollisionHandler collision_handler[ColliderType::SIZE][ColliderType::SIZE] = {
                // Circle = 0
                {
                        // Circle vs Circle
                        [](flecs::entity &a, const Collider *a_col, CollisionInfo &a_info,
                           flecs::entity &b, const Collider *b_col, CollisionInfo &b_info) {
                            return handle_circle_circle(a, a_col, a_info, b, b_col, b_info);
                        },
                        // Circle vs Box
                        [](flecs::entity &a, const Collider *a_col, CollisionInfo &a_info,
                           flecs::entity &b, const Collider *b_col, CollisionInfo &b_info) {
                            return handle_circle_rec_collision(a, a_col, a_info, b, b_col, b_info);
                        },
                },
                // Box = 1
                {
                        // Box vs Circle
                        [](flecs::entity &a, const Collider *a_col, CollisionInfo &a_info,
                           flecs::entity &b, const Collider *b_col, CollisionInfo &b_info) {
                            return handle_circle_rec_collision(b, b_col, b_info, a, a_col, a_info);
                        },
                        // Box vs Box
                        [](flecs::entity &a, const Collider *a_col, CollisionInfo &a_info,
                           flecs::entity &b, const Collider *b_col, CollisionInfo &b_info) {
                            return handle_boxes_collision(a, a_col, b, b_col);
                        },
                }
                // more collisions
        };
        /////////////////////

        inline void reset_desired_velocity(const Velocity &vel, DesiredVelocity &desired_vel) {
            desired_vel.value = vel.value;
        };

        inline void integrate_velocity(flecs::iter &it, size_t, Velocity &vel,
                                       const DesiredVelocity &desired_vel,
                                       const AccelerationSpeed &acc) {
            float dt = std::min(PHYSICS_TICK_LENGTH, it.delta_system_time());
            vel.value = vel.value + dt * acc.value * (desired_vel.value - vel.value);
        };

        inline void integrate_position(flecs::iter &it, size_t, core::Position &pos,
                                       const Velocity &vel) {
            pos.value += vel.value * std::min(PHYSICS_TICK_LENGTH, it.delta_system_time());
        };

        inline void detect_collision(flecs::entity body_a, CollisionRecordList &list,
                                     const core::Position &x_a, const Collider &collider_a) {
            flecs::world staged_world = body_a.world();

            auto query_all_visibles =
                    staged_world.query_builder<const core::Position, const Collider>()
                            .without<StaticCollider>()
                            .build();

            query_all_visibles.each([&](flecs::entity body_b, const core::Position &x_b,
                                        const Collider &collider_b) {
                // if (body_a.id() > body_b.id())
                //     return;
                if ((collider_a.collision_filter & collider_b.collision_type) == none)
                    return;

                Rectangle rect_a = {x_a.value.x() + collider_a.bounds.x,
                                    x_a.value.y() + collider_a.bounds.y, collider_a.bounds.width,
                                    collider_a.bounds.height};

                Rectangle rect_b = {x_b.value.x() + collider_b.bounds.x,
                                    x_b.value.y() + collider_b.bounds.y, collider_b.bounds.width,
                                    collider_b.bounds.height};

                if (CheckCollisionRecs(rect_a, rect_b)) {
                    list.records.push_back({body_a, body_b});
                }
            });
        };

        inline void detect_collision_alt(flecs::entity body_a, CollisionRecordList &list,
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

                // float radius = collider_a.radius + collider_b.radius;
                // float distance_sqr = (x_a.value - x_b.value).squaredNorm();
                // if (distance_sqr < radius * radius) {
                //     // collision between different types is treated `significant`
                //     if ((collider_a.collision_type & collider_b.collision_type) == none) {
                //         // these are for attack/damage/interaction logic
                //         // will be consumed in `CollidedWith` creation
                //         list.significant_collisions.push_back({body_a, body_b});
                //     }

                //     // correct_position? is this for static/dynamic body?
                //     if (collider_a.correct_position && collider_b.correct_position) {
                //         // these go to collision resolution
                //         list.records.push_back({body_a, body_b});
                //     }
                // }

                Rectangle rect_a = {x_a.value.x() + collider_a.bounds.x,
                                    x_a.value.y() + collider_a.bounds.y, collider_a.bounds.width,
                                    collider_a.bounds.height};

                Rectangle rect_b = {x_b.value.x() + collider_b.bounds.x,
                                    x_b.value.y() + collider_b.bounds.y, collider_b.bounds.width,
                                    collider_b.bounds.height};

                if (CheckCollisionRecs(rect_a, rect_b)) {
                    list.records.push_back({body_a, body_b});
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

                list.collisions_info[{a.id(), b.id()}] = record.a_info;
                list.collisions_info[{b.id(), a.id()}] = record.b_info;
            }
        }

        inline void resolve_collision(CollisionRecordList &list) {
            for (auto &record: list.records) {
                // auto a = record.a;
                // auto b = record.b;

                // auto x_a = a.try_get<core::Position>()->value;
                // auto x_b = b.try_get<core::Position>()->value;
                // auto v_a = a.try_get<Velocity>()->value;
                // auto v_b = b.try_get<Velocity>()->value;

                // float radius = a.try_get<Collider>()->radius + b.try_get<Collider>()->radius;

                // auto delta = x_a - x_b;
                // auto direction = delta.normalized();

                // auto grad_a = direction;
                // auto grad_b = -direction;

                // auto violation = radius - delta.norm();

                // auto correction_a = -grad_a * violation * 0.5f;
                // auto correction_b = -grad_b * violation * 0.5f;

                // v_a += correction_a;
                // v_b += correction_b;

                // x_a += correction_a / 2;
                // x_b += correction_b / 2;
                flecs::entity a = record.a; // Current entity
                flecs::entity b = record.b; // Colliding entity

                const Collider *a_col = a.try_get<Collider>();
                const Collider *b_col = b.try_get<Collider>();

                // are the entities colliding?
                CollisionInfo a_info;
                CollisionInfo b_info;
                if (!collision_handler[a_col->type][b_col->type](a, a_col, a_info, b, b_col,
                                                                 b_info))
                    continue;

                // if the entities are of different types (player & enemy) we report it a
                // significant collision enemy vs environment should not be significant. (too many
                // tables) But player vs environment should count (because of projectiles, they
                // might have behaviours specific to obstacles)
                if ((a_col->collision_type & b_col->collision_type) == none &&
                    (a_col->collision_type | b_col->collision_type) != (enemy | environment)) {
                    list.significant_collisions.push_back({a, b, a_info, b_info});
                }
            }

            // list.records.clear();
        }

        inline void delete_collision_relationship(flecs::iter &it, size_t i) {
            // auto a = it.entity(i);
            // auto b = it.pair(0).second();
            // a.remove<CollidedWith>(b);
            it.world().remove_all<CollidedWith>(it.entity(i));
        }

        inline void clean_collision_record(CollisionRecordList &list) {
            list.records.clear();
            list.significant_collisions.clear();
            list.collisions_info.clear();
        }

    } // namespace systems
} // namespace physics
