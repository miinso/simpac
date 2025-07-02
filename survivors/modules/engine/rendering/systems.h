#pragma once

#include "components.h"

#include <modules/engine/core/components.h>

#include <flecs.h>
#include <raygui.h>
#include <raylib.h>

namespace rendering {
    namespace systems {
        void inline begin_drawing(flecs::iter &it) {
            BeginDrawing();
            ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));
        };

        void inline determine_visible_entity(flecs::entity e, const core::Position &position,
                                             const Renderable &renderable,
                                             const core::GameSettings &settings) {
            auto x_above_upperbound =
                    position.value.x() > settings.windowWidth + renderable.texture.width;
            auto x_below_lowerbound = position.value.x() < -renderable.texture.width;
            auto y_above_upperbound =
                    position.value.y() > settings.windowHeight + renderable.texture.height;
            auto y_below_lowerbound = position.value.y() < -renderable.texture.height;

            if (x_below_lowerbound || x_above_upperbound || y_below_lowerbound ||
                y_above_upperbound) {
                e.remove<Visible>();
            } else if (!e.has<Visible>()) {
                e.add<Visible>();
            }
        };

        void inline draw_entity_with_texture(const Renderable &renderable,
                                             const core::Position &position,
                                             const Rotation *rotation) {
            Rectangle rec{0.0f, 0.0f, (float) renderable.texture.width,
                          (float) renderable.texture.height};

            float scaled_width = renderable.texture.width * renderable.scale;
            float scaled_height = renderable.texture.height * renderable.scale;

            Rectangle source{position.value.x() + renderable.draw_offset.x * renderable.scale,
                             position.value.y() + renderable.draw_offset.y * renderable.scale,
                             scaled_width, scaled_height};

            Vector2 origin = Vector2{scaled_width / 2.0f, scaled_height / 2.0f};

            float r = rotation ? rotation->angle : 0.0f; // degree
            DrawTexturePro(renderable.texture, rec, source, origin, r, renderable.tint);
        }

        void inline draw_health_bar(ProgressBar &bar, const core::Position &position,
                                    const Renderable &renderable) {
            if (bar.max_val - bar.current_val <= 0.05f)
                return;
            GuiProgressBar({position.value.x() - bar.rectangle.width / 2.0f,
                            position.value.y() - bar.rectangle.height - renderable.texture.height,
                            bar.rectangle.width, bar.rectangle.height},
                           "", "", &bar.current_val, 0, bar.max_val);
        }

        void inline end_drawing(flecs::iter &it) { EndDrawing(); }
    } // namespace systems
} // namespace rendering
