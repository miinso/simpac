#pragma once

#include <flecs.h>
#include <raylib.h>

#include "camera.h"
#include "input.h"
#include "lighting.h"
#include "pipelines.h"
#include "vars.h"

namespace graphics::systems {

inline void install_pipeline_systems(flecs::world& ecs) {
    // camera update in PreRender (before render passes)
    ecs.system<Position, Camera>("graphics::update_camera")
        .term_at(0).src("$cam")
        .term_at(1).src("$cam")
        .with<ActiveCamera>("$cam").src(camera::active_camera_source(ecs).id())
        .kind(PreRender)
        .each([](Position& pos, Camera& cam) {
            update_camera_controls(pos, cam);
        });

    // sync active camera to raylib before rendering
    ecs.system<const Position, const Camera>("graphics::sync_camera")
        .term_at(0).src("$cam")
        .term_at(1).src("$cam")
        .with<ActiveCamera>("$cam").src(camera::active_camera_source(ecs).id())
        .kind(PreRender)
        .each([](const Position& pos, const Camera& cam) {
            detail::raylib_camera = to_raylib(pos, cam);
        });

    // PreRender: begin drawing, clear, update lighting
    ecs.system("graphics::begin")
        .kind(PreRender)
        .run([](flecs::iter& it) {
            graphics::shim::update();

            if (IsWindowResized()) {
                SetWindowSize(GetScreenWidth(), GetScreenHeight());
            }

            BeginDrawing();
            ClearBackground(graphics::to_color(props::background_color.get<color4f>()));

            if (is_lighting_enabled()) {
                update_lighting_camera_pos(detail::raylib_camera.position);
            }
        });

    // OnRender: begin 3D mode
    ecs.system("graphics::render3d")
        .kind(OnRender)
        .run([](flecs::iter& it) {
            BeginMode3D(detail::raylib_camera);

            if (is_lighting_enabled()) {
                BeginShaderMode(get_lighting_shader());
            }
        });

    // PostRender: end 3D mode, draw overlays
    ecs.system("graphics::render2d")
        .kind(PostRender)
        .run([](flecs::iter& it) {
            if (is_lighting_enabled()) {
                EndShaderMode();
            }

            if (props::show_grid.get<bool>()) {
                DrawGrid(props::grid_slices.get<int>(), props::grid_spacing.get<float>());
            }

            EndMode3D();

            if (props::show_fps.get<bool>()) {
                DrawFPS(20, 20);
            }
        });

    // OnPresent: end drawing
    ecs.system("graphics::end")
        .kind(OnPresent)
        .run([](flecs::iter& it) {
            EndDrawing();
        });
}

} // namespace graphics::systems
