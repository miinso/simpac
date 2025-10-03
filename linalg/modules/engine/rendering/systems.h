#pragma once

#include "components.h"

#include <modules/engine/core/components.h>

#include <cstddef>
#include <flecs.h>
#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>

namespace rendering {
    namespace systems {
        inline void create_camera(TrackingCamera &camera) {
            if (!camera.target.is_alive()) {
                camera.camera.zoom = 1.0f;
                camera.camera.rotation = 0.0f;
                return;
            }

            if (const auto *position = camera.target.try_get<core::Position>()) {
                camera.camera.target = {position->value.x(), position->value.y()};
            }

            const Vector2 screen_half_extent{
                static_cast<float>(GetScreenWidth()) * 0.5f,
                static_cast<float>(GetScreenHeight()) * 0.5f,
            };

            camera.camera.offset = screen_half_extent;
            camera.camera.zoom = 1.0f;
            camera.camera.rotation = 0.0f;
        }

        inline void begin_drawing([[maybe_unused]] flecs::iter &it) {
            BeginDrawing();
            ClearBackground(RAYWHITE);
        }

        inline void begin_camera_mode(flecs::iter &it,
                                      [[maybe_unused]] std::size_t,
                                      TrackingCamera &camera,
                                      core::Settings &settings) {
            Vector2 target = camera.camera.target;
            if (camera.target.is_alive()) {
                if (const auto *position = camera.target.try_get<core::Position>()) {
                    target = {position->value.x(), position->value.y()};
                }
            }

            camera.camera.target = Vector2Lerp(camera.camera.target, target, it.delta_time() * 2.0f);
            camera.camera.offset.x = static_cast<float>(settings.window_width) * 0.5f;
            camera.camera.offset.y = static_cast<float>(settings.window_height) * 0.5f;
            BeginMode2D(camera.camera);

            rlPushMatrix();
            rlScalef(1.0f, -1.0f, 1.0f);
        }

        inline void draw_particle(const core::particle_q &q,
                                  const core::particle_qd &qd,
                                  const core::particle_radius *radius) {
            const float render_radius = radius ? radius->value : 10.0f;
            const auto x = q.value;
            const auto v = qd.value;
            DrawCircleLines(x.x(), x.y(), render_radius, MAGENTA);
            DrawLine(x.x(), x.y(), (x + v).x(), (x + v).y(), RED);
        }

        inline void draw_particle2(const Eigen::Vector2f &x) {
            constexpr float kParticleOutline = 0.05f;
            DrawCircleLines(x.x(), x.y(), kParticleOutline, GREEN);
        }

        inline void end_camera_mode([[maybe_unused]] flecs::iter &it) {
            rlPopMatrix();
            EndMode2D();
        }

        inline void end_drawing([[maybe_unused]] flecs::iter &it) { EndDrawing(); }
    } // namespace systems
} // namespace rendering
