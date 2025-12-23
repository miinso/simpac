#pragma once

#include <flecs.h>

#include <modules/engine/core/components.h>
#include <modules/engine/rendering/components.h>
#include "components.h"
#include "physics_module.h"

using namespace core;
using namespace Eigen;

namespace physics {
    namespace systems {
        inline void integrate_particle(flecs::iter &it, size_t, particle_q &q, particle_qd &qd,
                                       particle_f &f, particle_inv_mass &w,
                                       // global
                                       gravity &gravity, v_max &v_max) {
            float dt = PHYSICS_TICK_LENGTH;
            // float dt = it.delta_system_time();
            auto x0 = q.value;
            auto v0 = qd.value;
            auto f0 = f.value;
            auto inv_mass = w.value;
            auto g = gravity.value;

            // simple semi-implicit euler. v1 = v0 + dt * f(x0, v0); x1 = x0 + dt * v1;
            Vector2f v1 = v0 + (dt * f0 * inv_mass) + (dt * g * (inv_mass > 0 ? 1 : 0));

            // auto v1_mag = v1.norm();
            // if (v1_mag > v_max.value) {
            //     v1 *= v_max.value / v1_mag;
            // }
            auto x1 = x0 + dt * v1;

            // v0 = v1;
            // x0 = x1;

            qd.value = v1;
            q.value = x1;
        };

        // inline void step(flecs::iter &it, size_t, state &state_in, state &state_out, float dt) {}

        // inline void integrate_particle_position(flecs::iter &it, size_t, core::Position &pos,
        //                                         const Velocity &vel) {
        //     float dt = std::min(PHYSICS_TICK_LENGTH, it.delta_system_time());
        //     // float dt = PHYSICS_TICK_LENGTH;
        //     pos.value += dt * vel.value;
        // };

        // static??
        // inline void detect_collision(flecs::entity body_a, CollisionRecordList &list,
        //                              const core::Position &x_a, const Collider &collider_a) {
        //     // baseline, O(n^2)
        //     flecs::world staged_world = body_a.world();

        //     // what does `staging` do?
        //     // https://www.flecs.dev/flecs/md_docs_2Systems.html#staging
        //     auto query_all_visibles =
        //             staged_world.query_builder<const core::Position, const Collider>()
        //                     .without<StaticCollider>()
        //                     .build();

        //     query_all_visibles.each([&](flecs::entity body_b, const core::Position &x_b,
        //                                 const Collider &collider_b) {
        //         // if (body_a.id() > body_b.id())
        //         //     return;
        //         if ((collider_a.collision_filter & collider_b.collision_type) == none)
        //             return;

        //         Rectangle rect_a = {x_a.value.x() + collider_a.bounds.x,
        //                             x_a.value.y() + collider_a.bounds.y, collider_a.bounds.width,
        //                             collider_a.bounds.height};

        //         Rectangle rect_b = {x_b.value.x() + collider_b.bounds.x,
        //                             x_b.value.y() + collider_b.bounds.y, collider_b.bounds.width,
        //                             collider_b.bounds.height};

        //         if (CheckCollisionRecs(rect_a, rect_b)) {
        //             list.records.push_back({body_a, body_b});
        //         }
        //     });
        // };

        // non-static??
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


        inline void resolve_collision(CollisionRecordList &list) {
            for (auto &record: list.records) {
                flecs::entity a = record.a; // Current entity
                flecs::entity b = record.b; // Colliding entity

                const auto *a_col = a.try_get<Collider>();
                const auto *b_col = b.try_get<Collider>();

                if (!a_col || !b_col)
                    continue;

                CollisionInfo a_info, b_info;
                bool collision_occurred = false;

                // if the entities are of different types (player & enemy) we report it a
                // significant collision enemy vs environment should not be significant. (too many
                // tables) But player vs environment should count (because of projectiles, they
                // might have behaviours specific to obstacles)
                if (collision_occurred) {
                    if ((a_col->collision_type & b_col->collision_type) == none &&
                        (a_col->collision_type | b_col->collision_type) != (enemy | environment)) {
                        list.significant_collisions.push_back({a, b, a_info, b_info});
                    }
                }
            }
        }

        inline void clean_collision_record(CollisionRecordList &list) {
            list.records.clear();
            list.significant_collisions.clear();
            list.collisions_info.clear();
        }
    } // namespace systems
} // namespace physics
