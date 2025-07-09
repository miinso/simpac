#pragma once

#include <modules/engine/core/components.h>
#include <modules/engine/core/queries.h>
#include <modules/engine/physics/components.h>
#include <modules/engine/rendering/queries.h>

#include <raygui.h>
#include <raylib.h>

namespace debug {
    namespace systems {
        inline void draw_collidable(flecs::entity e, const core::Position &pos,
                                    const physics::Collider &col) {
            DrawText(TextFormat("%d", e.id()), pos.value.x() + col.bounds.x + 12,
                     pos.value.y() + col.bounds.y + 12, 16, GREEN);
            DrawCircleLines(pos.value.x(), pos.value.y(), 0.2f, RED);
        }

        inline void draw_collider_bound(const core::Position &pos, const physics::Collider &col) {
            DrawRectangleLines(pos.value.x() + col.bounds.x, pos.value.y() + col.bounds.y,
                               col.bounds.width, col.bounds.height, MAGENTA);
        }

        inline void draw_circle_collider(const core::Position &pos,
                                         const physics::CircleCollider &col) {
            DrawCircleLines(pos.value.x(), pos.value.y(), col.radius, GREEN);
        }

        inline void draw_box_collider(const core::Position &pos, const physics::Collider &col) {
            DrawRectangleLines(pos.value.x() + col.bounds.x, pos.value.y() + col.bounds.y,
                               col.bounds.width, col.bounds.height, GREEN);
        }

        inline void draw_fps(flecs::iter &iter) {
            DrawRectangleRec({0, 10, 225, 20}, DARKGRAY);
            DrawFPS(10, 10);
        };

        inline void draw_entity_count(flecs::iter &it) {
            DrawRectangleRec({0, 30, 225, 60}, DARKGRAY);
            auto renderable_count = rendering::queries::query_all_renderables.count();
            auto collidable_count = it.world().query<physics::Collider>().count();
            auto visible_renderable_count = rendering::queries::query_visible_renderables.count();
            DrawText(TextFormat("%d entities", renderable_count), 10, 30 + 0, 20, GREEN);
            DrawText(TextFormat("%d visible entities", visible_renderable_count), 10, 30 + 20, 20,
                     GREEN);
            DrawText(TextFormat("%d collidable entities", visible_renderable_count), 10, 30 + 60,
                     20, GREEN);
        };

        inline void draw_mouse_position(flecs::iter &iter) {
            DrawCircle(GetMouseX(), GetMouseY(), 10, RED);
        };

        inline void draw_grid(flecs::iter &iter) {
            GuiGrid({0, 0, (float) GetScreenWidth(), (float) GetScreenHeight()}, "grid", 48, 1,
                    nullptr);
        };

        inline void draw_closest_enemy(flecs::iter &it) {
            auto player = it.world().lookup("player");
            auto pos = player.try_get<core::Position>();
            float shortest_distance_sqr = 1e7;
            core::Position target_pos{pos->value};

            core::queries::position_and_tag().each(
                    [&](const core::Position &other_pos, const core::Tag &tag) {
                        if ("enemy" != tag.name)
                            return;

                        float d = (pos->value - other_pos.value).squaredNorm();

                        if (d > shortest_distance_sqr)
                            return;

                        shortest_distance_sqr = d;
                        target_pos = other_pos;
                    });

            if (target_pos.value == pos->value)
                return;

            DrawLineEx(Vector2{pos->value.x(), pos->value.y()},
                       Vector2{target_pos.value.x(), target_pos.value.y()}, 1, GREEN);
        };
    } // namespace systems
} // namespace debug
