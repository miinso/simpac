#pragma once

#include <modules/engine/core/components.h>
#include <modules/engine/core/queries.h>
#include <modules/engine/physics/components.h>
#include <modules/engine/rendering/queries.h>

#include <raygui.h>
#include <raylib.h>

namespace debug {
    namespace systems {
        inline void draw_collider(const physics::Collider &collider,
                                  const core::Position &position) {
            DrawCircleLines(position.value.x(), position.value.y(), collider.radius, GREEN);
        };

        inline void draw_fps(flecs::iter &iter) {
            DrawRectangleRec({0, 10, 225, 20}, DARKGRAY);
            DrawFPS(10, 10);
        };

        inline void draw_entity_count(flecs::iter &iter) {
            DrawRectangleRec({0, 30, 225, 40}, DARKGRAY);
            auto renderable_count = rendering::queries::query_all_renderables().count();
            auto visible_renderable_count = rendering::queries::query_visible_renderables().count();
            DrawText(std::string(std::to_string(renderable_count) + " entities").c_str(), 10, 30,
                     20, GREEN);
            DrawText(std::string(std::to_string(visible_renderable_count) + " visible entities")
                             .c_str(),
                     10, 50, 20, GREEN);
        };

        inline void draw_mouse_position(flecs::iter &iter) {
            DrawCircle(GetMouseX(), GetMouseY(), 10, RED);
        };

        inline void draw_grid(flecs::iter &iter) {
            GuiGrid({0, 0, (float) GetScreenWidth(), (float) GetScreenHeight()}, "grid", 32, 1,
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
