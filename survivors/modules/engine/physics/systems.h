#pragma once

#include <flecs.h>

#include <modules/engine/rendering/components.h>

#include "components.h"
#include "physics_module.h"

namespace physics {

    namespace systems {
        namespace detail {
            inline bool check_collision_filter(const Collider &a, const Collider &b) {
                return (a.collision_filter & b.collision_type) != none;
            }

            inline Rectangle get_world_bounds(const core::Position &pos, const Collider &col) {
                return {pos.value.x() + col.bounds.x, pos.value.y() + col.bounds.y,
                        col.bounds.width, col.bounds.height};
            }

            static Vector2 collide_circle_circle(const CircleCollider &a,
                                                 const core::Position &a_pos, CollisionInfo &a_info,
                                                 const CircleCollider &b,
                                                 const core::Position &b_pos,
                                                 CollisionInfo &b_info) {
                float combinedRadius = a.radius + b.radius;

                // Find the distance and adjust to resolve the overlap
                auto delta = (b_pos.value - a_pos.value);
                auto direction = delta.normalized();
                // Vector2 moveDirection = Vector2Normalize(direction);
                // float overlap = combinedRadius - Vector2Length(direction);
                float overlap = combinedRadius - delta.norm();

                a_info.normal = Vector2Negate({direction.x(), direction.y()});
                b_info.normal = {direction.x(), direction.y()};

                auto correction = direction * overlap;
                return {correction.x(), correction.y()};
            }

            static Vector2 collide_circle_box(const CircleCollider &a, core::Position &a_pos,
                                              CollisionInfo &a_info, const Collider &b,
                                              core::Position &b_pos, CollisionInfo &b_info) {
                float recCenterX = b_pos.value.x() + b.bounds.x + b.bounds.width / 2.0f;
                float recCenterY = b_pos.value.y() + b.bounds.y + b.bounds.height / 2.0f;

                float halfWidth = b.bounds.width / 2.0f;
                float halfHeight = b.bounds.height / 2.0f;

                float dx = a_pos.value.x() - recCenterX;
                float dy = a_pos.value.y() - recCenterY;

                float absDx = fabsf(dx);
                float absDy = fabsf(dy);

                Vector2 overlap = {0, 0};

                if (absDx > (halfWidth + a.radius))
                    return overlap;
                if (absDy > (halfHeight + a.radius))
                    return overlap;

                if (absDx <= halfWidth || absDy <= halfHeight) {
                    // Side collision — resolve with axis-aligned MTV
                    float overlapX = (halfWidth + a.radius) - absDx;
                    float overlapY = (halfHeight + a.radius) - absDy;


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
                float radius = a.radius;

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

            inline void correct_positions(flecs::entity &a, const Collider &a_col,
                                          CollisionInfo &a_info, flecs::entity &b,
                                          const Collider &b_col, CollisionInfo &b_info,
                                          Vector2 overlap) {
                core::Position *a_pos = a.try_get_mut<core::Position>();
                core::Position *b_pos = b.try_get_mut<core::Position>();

                float a_move_ratio = 0.5f;
                float b_move_ratio = 0.5f;

                if (a_col.static_body) {
                    a_move_ratio = 0;
                    b_move_ratio = 1.0f;
                }
                if (b_col.static_body) {
                    a_move_ratio = 1.0f;
                    b_move_ratio = 0;
                }

                if ((!a_col.correct_position && !a_col.static_body) ||
                    (!b_col.correct_position && !b_col.static_body)) {
                    a_move_ratio = 0.0f;
                    b_move_ratio = 0.0f;
                }

                auto correction = Eigen::Vector3f(overlap.x, overlap.y, 0);
                a_pos->value = a_pos->value - correction * a_move_ratio * 0.9f;
                b_pos->value = b_pos->value + correction * b_move_ratio * 0.9f;
            }

            inline bool handle_circle_circle_collision(flecs::entity &a, const Collider &a_col,
                                                       CollisionInfo &a_info, flecs::entity &b,
                                                       const Collider &b_col,
                                                       CollisionInfo &b_info) {
                auto a_pos = a.try_get<core::Position>()->value;
                auto b_pos = b.try_get<core::Position>()->value;

                const auto col = a.get<CircleCollider>(); // this will panic if fails
                const auto other_col = b.get<CircleCollider>();

                // broad
                if (!CheckCollisionCircles({a_pos.x(), a_pos.y()}, col.radius,
                                           {b_pos.x(), b_pos.y()}, other_col.radius)) {
                    return false;
                }

                // narrow
                Vector2 overlap =
                        detail::collide_circle_circle(col, a.get<core::Position>(), a_info,
                                                      other_col, b.get<core::Position>(), b_info);

                // resolve
                correct_positions(a, a_col, a_info, b, b_col, b_info, overlap);

                Vector2 contact_point = Vector2{a_pos.x(), a_pos.y()} + b_info.normal * col.radius;
                a_info.contact_point = contact_point;
                b_info.contact_point = contact_point;

                return true;
            }

            inline bool handle_circle_box_collision(flecs::entity &a, const Collider &a_col,
                                                    CollisionInfo &a_info, flecs::entity &b,
                                                    const Collider &b_col, CollisionInfo &b_info) {
                auto a_pos = a.try_get<core::Position>()->value;
                auto b_pos = b.try_get<core::Position>()->value;
                const auto circle_col =
                        a.get<CircleCollider>(); // will panic if no `CircleCollider` is attached

                if (!CheckCollisionCircleRec({a_pos.x(), a_pos.y()}, circle_col.radius,
                                             {b_pos.x() + b_col.bounds.x,
                                              b_pos.y() + b_col.bounds.y, b_col.bounds.width,
                                              b_col.bounds.height})) {
                    return false;
                }

                Vector2 overlap =
                        detail::collide_circle_box(circle_col, a.get_mut<core::Position>(), a_info,
                                                   b_col, b.get_mut<core::Position>(), b_info);

                correct_positions(a, a_col, a_info, b, b_col, b_info, overlap);

                Vector2 contact_point =
                        Vector2{a_pos.x(), a_pos.y()} + b_info.normal * circle_col.radius;
                a_info.contact_point = contact_point;
                b_info.contact_point = contact_point;

                return true;
            }

            inline bool handle_box_box_collision(flecs::entity &a, const Collider &a_col,
                                                 CollisionInfo &a_info, flecs::entity &b,
                                                 const Collider &b_col, CollisionInfo &b_info) {
                auto a_pos = a.try_get<core::Position>()->value;
                auto b_pos = b.try_get<core::Position>()->value;

                if (!CheckCollisionRecs({a_pos.x() + a_col.bounds.x, a_pos.y() + a_col.bounds.y,
                                         a_col.bounds.width, a_col.bounds.height},
                                        {b_pos.x() + b_col.bounds.x, b_pos.y() + b_col.bounds.y,
                                         b_col.bounds.width, b_col.bounds.height})) {
                    return false;
                }

                // TODO: implment
                return true;
            }

            inline bool resolve_collision_by_type(flecs::entity &a, flecs::entity &b,
                                                  const Collider &a_col, const Collider &b_col,
                                                  CollisionInfo &a_info, CollisionInfo &b_info) {
                ColliderType a_type = a_col.type;
                ColliderType b_type = b_col.type;

                if (a_type == ColliderType::Circle && b_type == ColliderType::Circle) {
                    return handle_circle_circle_collision(a, a_col, a_info, b, b_col, b_info);
                } else if (a_type == ColliderType::Circle && b_type == ColliderType::Box) {
                    return handle_circle_box_collision(a, a_col, a_info, b, b_col, b_info);
                } else if (a_type == ColliderType::Box && b_type == ColliderType::Circle) {
                    return handle_circle_box_collision(b, b_col, b_info, a, a_col, a_info);
                } else if (a_type == ColliderType::Box && b_type == ColliderType::Box) {
                    return handle_box_box_collision(a, a_col, a_info, b, b_col, b_info);
                }

                return false;
            }

        } // namespace detail

        ////////////////////////////////////////////////////////////////////////////////////////////
        inline void init_hash_grid(flecs::iter &it, size_t i, HashGrid &grid,
                                   core::GameSettings &settings) {
            const auto num_cell_x =
                    std::ceil((float) settings.initial_width) / (float) grid.cell_size;
            const auto num_cell_y =
                    std::ceil((float) settings.initial_height) / (float) grid.cell_size;
            for (int y = -1; y < num_cell_y + 1; y++) {
                for (int x = -1; x < num_cell_x + 1; x++) {
                    auto e = it.world().entity().set<GridCell>({x, y});
                    grid.cells[std::make_pair(x, y)] = e;
                }
            }
        }

        inline void empty_hash_grid(flecs::iter &it, size_t i, HashGrid &grid,
                                    core::GameSettings &settings) {
            // cell.first: key, cell.second: flecs::entity
            for (auto cell: grid.cells) {
                cell.second.destruct();
            }
            grid.cells.clear();
        }

        inline void reset_hash_grid(flecs::iter &it, size_t i, HashGrid &grid,
                                    core::GameSettings &settings) {
            empty_hash_grid(it, i, grid, settings);
            init_hash_grid(it, i, grid, settings);
        }

        inline void reset_hash_grid_on_window_resize(flecs::iter &it, size_t i, HashGrid &grid,
                                                     core::GameSettings &settings) {
            if (IsWindowResized()) {
                reset_hash_grid(it, i, grid, settings);
            }
        }

        inline void empty_hash_grid_cell_on_camera_move(flecs::iter &it, size_t i, HashGrid &grid,
                                                        rendering::TrackingCamera &camera,
                                                        core::GameSettings &settings,
                                                        GridCell &cell) {
            auto screen_center =
                    Vector2{settings.window_width / 2.0f, settings.window_height / 2.0f};
            grid.offset = camera.camera.target - screen_center;

            cell.items.clear(); // vacant the cell
        }

        inline void populate_hash_grid_cell(flecs::entity e, HashGrid &grid, Collider &collider,
                                            core::Position &pos) {
            int cell_pos_x = std::floor((pos.value.x() - grid.offset.x) / grid.cell_size);
            int cell_pos_y = std::floor((pos.value.y() - grid.offset.y) / grid.cell_size);

            auto key = std::make_pair(cell_pos_x, cell_pos_y);

            if (!grid.cells.count(key)) {
                return;
            }

            // query the cell for given position
            auto cell = grid.cells[key];
            cell.get_mut<GridCell>().items.push_back(e);
            e.add<ContainedIn>(cell);
        }

        inline void detect_collision_with_hash_grid(flecs::entity e, CollisionRecordList &list,
                                                    HashGrid &grid, GridCell &cell) {
            for (int offset_y = -1; offset_y <= 1; offset_y++) {
                for (int offset_x = -1; offset_x <= 1; offset_x++) {
                    int x = cell.x + offset_x;
                    int y = cell.y + offset_y;
                    auto key = std::make_pair(x, y);

                    if (!grid.cells.count(key))
                        continue;

                    const GridCell neighbour = grid.cells[key].get<GridCell>();

                    for (int i = 0; i < cell.items.size(); i++) {
                        flecs::entity self = cell.items[i];

                        if (!self.is_alive())
                            continue;

                        const core::Position pos = cell.items[i].get<core::Position>();
                        const Collider collider = cell.items[i].get<Collider>();

                        for (int j = 0; j < neighbour.items.size(); j++) {
                            flecs::entity other = neighbour.items[j];

                            if (!other.is_alive())
                                continue;

                            const core::Position other_pos =
                                    neighbour.items[j].get<core::Position>();
                            const Collider other_collider = neighbour.items[j].get<Collider>();

                            if (self.id() <= other.id())
                                continue;

                            if ((collider.collision_filter & other_collider.collision_type) == none)
                                continue;

                            Rectangle self_rec = {pos.value.x() + collider.bounds.x,
                                                  pos.value.y() + collider.bounds.y,
                                                  collider.bounds.width, collider.bounds.height};
                            Rectangle other_rec = {other_pos.value.x() + other_collider.bounds.x,
                                                   other_pos.value.y() + other_collider.bounds.y,
                                                   other_collider.bounds.width,
                                                   other_collider.bounds.height};

                            if (CheckCollisionRecs(self_rec, other_rec)) {
                                list.records.push_back({self, other});
                            }
                        }
                    }
                }
            }
        }

        inline void detect_collision_with_hash_grid_alt(flecs::iter &it, size_t i,
                                                        CollisionRecordList &list, HashGrid &grid,
                                                        GridCell &cell) {

            flecs::entity current_cell = it.entity(i);
            auto cur_q = it.world()
                                 .query_builder<const core::Position, const Collider>()
                                 .with<ContainedIn>(current_cell)
                                 .filter();

            for (int offset_y = -1; offset_y <= 1; offset_y++) {
                for (int offset_x = -1; offset_x <= 1; offset_x++) {
                    int x = cell.x + offset_x;
                    int y = cell.y + offset_y;
                    auto key = std::make_pair(x, y);

                    if (!grid.cells.count(key))
                        continue;

                    auto pair = std::make_pair(x, y);
                    flecs::entity neighbour_cell = grid.cells[pair];

                    auto other_q = it.world()
                                           .query_builder<const core::Position, const Collider>()
                                           .with<ContainedIn>(neighbour_cell)
                                           .filter();

                    cur_q.each([&](flecs::iter &self_it, size_t self_i, const core::Position &pos,
                                   const Collider &collider) {
                        flecs::entity self = self_it.entity(self_i);

                        other_q.each([&](flecs::iter &other_it, size_t other_i,
                                         const core::Position &other_pos,
                                         const Collider &other_collider) {
                            flecs::entity other = other_it.entity(other_i);
                            if (other.id() <= self.id())
                                return;

                            if ((collider.collision_filter & other_collider.collision_type) == none)
                                return;

                            Rectangle self_rec = {pos.value.x() + collider.bounds.x,
                                                  pos.value.y() + collider.bounds.y,
                                                  collider.bounds.width, collider.bounds.height};
                            Rectangle other_rec = {other_pos.value.x() + other_collider.bounds.x,
                                                   other_pos.value.y() + other_collider.bounds.y,
                                                   other_collider.bounds.width,
                                                   other_collider.bounds.height};

                            if (CheckCollisionRecs(self_rec, other_rec)) {
                                list.records.push_back({self, other});
                            }
                        });
                    });
                }
            }
        }

        ////////////////////////////////////////////////////////////////////////////////////////////

        inline void reset_desired_velocity(const Velocity &vel, DesiredVelocity &desired_vel) {
            desired_vel.value = vel.value;
        };

        inline void integrate_velocity(flecs::iter &it, size_t, Velocity &vel,
                                       const DesiredVelocity &desired_vel,
                                       const AccelerationSpeed &acc) {
            float dt = std::min(PHYSICS_TICK_LENGTH, it.delta_system_time());
            // float dt = PHYSICS_TICK_LENGTH;
            vel.value += dt * acc.value * (desired_vel.value - vel.value);

            // std::printf("%f\n", it.delta_system_time());
        };

        inline void integrate_position(flecs::iter &it, size_t, core::Position &pos,
                                       const Velocity &vel) {
            float dt = std::min(PHYSICS_TICK_LENGTH, it.delta_system_time());
            // float dt = PHYSICS_TICK_LENGTH;
            pos.value += dt * vel.value;
        };

        // static??
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
                flecs::entity a = record.a; // Current entity
                flecs::entity b = record.b; // Colliding entity

                const auto *a_col = a.try_get<Collider>();
                const auto *b_col = b.try_get<Collider>();

                if (!a_col || !b_col)
                    continue;

                CollisionInfo a_info, b_info;
                bool collision_occurred =
                        detail::resolve_collision_by_type(a, b, *a_col, *b_col, a_info, b_info);

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

        inline void delete_collision_relationship(flecs::iter &it, size_t i) {
            it.world().remove_all<CollidedWith>(it.entity(i));
        }

        inline void clean_collision_record(CollisionRecordList &list) {
            list.records.clear();
            list.significant_collisions.clear();
            list.collisions_info.clear();
        }
    } // namespace systems
} // namespace physics
