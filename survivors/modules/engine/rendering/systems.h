#pragma once

#include "components.h"

#include <modules/engine/core/components.h>

#include <flecs.h>
#include <raygui.h>
#include <raylib.h>
#include <raymath.h>


namespace rendering {
    namespace systems {
        void inline create_camera(TrackingCamera &camera) {
            if (!camera.target.is_alive())
                return;
            auto camera_pos = camera.target.try_get<core::Position>()->value;
            camera.camera.target = {camera_pos.x(), camera_pos.y()};
            camera.camera.offset = {GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f};
            camera.camera.zoom = 1.0f;
            camera.camera.rotation = 0;
        }

        void inline begin_drawing(flecs::iter &it) {
            BeginDrawing();
            ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));
        };

        inline void begin_camera_mode(flecs::iter &it, size_t, TrackingCamera &camera,
                                      core::GameSettings &settings) {
            Vector2 pos = camera.camera.target;
            if (camera.target.is_alive()) {
                auto camera_pos = camera.target.try_get<core::Position>()->value;
                pos = Vector2{camera_pos.x(), camera_pos.y()};
            };
            camera.camera.target = Vector2Lerp(camera.camera.target, Vector2{pos.x, pos.y},
                                               it.delta_time() * 2.0f);
            camera.camera.offset.x = settings.window_width / 2.0f;
            camera.camera.offset.y = settings.window_height / 2.0f;
            BeginMode2D(camera.camera);
        }

        void inline determine_visible_entity(flecs::entity e, const core::Position &position,
                                             const Renderable &renderable,
                                             const core::GameSettings &settings,
                                             const TrackingCamera &camera) {
            bool x_above_upperbound = position.value.x() - camera.camera.target.x >
                                      settings.window_width * 1.05f + renderable.texture.width -
                                              camera.camera.offset.x;
            bool x_below_lowerbound =
                    position.value.x() - camera.camera.target.x + settings.window_width * 0.05f <
                    -renderable.texture.width - camera.camera.offset.x;

            bool y_above_upperbound = position.value.y() - camera.camera.target.y >
                                      settings.window_height * 1.05f + renderable.texture.height -
                                              camera.camera.offset.y;
            bool y_below_lowerbound =
                    position.value.y() - camera.camera.target.y + settings.window_height * 0.05f <
                    -renderable.texture.height - camera.camera.offset.y;

            // auto x_above_upperbound =
            //         position.value.x() > settings.window_width + renderable.texture.width;
            // auto x_below_lowerbound = position.value.x() < -renderable.texture.width;
            // auto y_above_upperbound =
            //         position.value.y() > settings.window_height + renderable.texture.height;
            // auto y_below_lowerbound = position.value.y() < -renderable.texture.height;

            if (x_below_lowerbound || x_above_upperbound || y_below_lowerbound ||
                y_above_upperbound) {
                e.remove<Visible>();
            } else if (!e.has<Visible>()) {
                e.add<Visible>();
            }
        };

        void inline draw_entity_with_texture(const Renderable &renderable, const core::Position &position,
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
            DrawTexturePro(renderable.texture, rec, source, origin + renderable.draw_offset, r,
                           renderable.tint);
        }

        inline void draw_background_texture(const Renderable &renderable) {
            Rectangle rec{0.0f, 0.0f, (float) renderable.texture.width,
                          (float) renderable.texture.height};

            float scaledWidth = renderable.texture.width * renderable.scale;
            float scaledHeight = renderable.texture.height * renderable.scale;

            Rectangle source{renderable.draw_offset.x * renderable.scale,
                             renderable.draw_offset.y * renderable.scale, scaledWidth,
                             scaledHeight};

            Vector2 origin = Vector2{scaledWidth / 2.0f, scaledHeight / 2.0f};

            DrawTexture(renderable.texture, 0, 0, renderable.tint);
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

        inline void end_camera_mode(flecs::iter &it) { EndMode2D(); }

        void inline end_drawing(flecs::iter &it) { EndDrawing(); }
    } // namespace systems
} // namespace rendering
