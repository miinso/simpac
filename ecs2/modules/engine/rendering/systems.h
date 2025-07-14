#pragma once

#include "components.h"

#include <modules/engine/core/components.h>

#include <flecs.h>
// #include <raygui.h>
#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>

using namespace core;

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
            ClearBackground(RAYWHITE);
            // ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));
        };

        inline void begin_camera_mode(flecs::iter &it, size_t, TrackingCamera &camera,
                                      core::Settings &settings) {
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

            rlPushMatrix();
            rlScalef(1.0f, -1.0f, 1.0f); // flip y coord to make it y-up
            // rlTranslatef(0.0f, -settings.window_height, 0.0f);
        }

        inline void draw_particle(const particle_q &q, const particle_qd &qd,
                                  const particle_radius *radius) {
            // `radius` here is optional
            // y-up,
            auto x = q.value;
            auto v = qd.value;
            DrawCircleLines(x.x(), x.y(), 10, MAGENTA);
            DrawLine(x.x(), x.y(), (x + v).x(), (x + v).y(), RED);
        }

        inline void draw_particle2(const Eigen::Vector2f &x) {
            DrawCircleLines(x.x(), x.y(), 0.05f, GREEN);
        }

        inline void end_camera_mode(flecs::iter &it) {
            rlPopMatrix();
            EndMode2D();
        }

        void inline end_drawing(flecs::iter &it) { EndDrawing(); }
    } // namespace systems
} // namespace rendering
