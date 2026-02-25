#pragma once

#include <flecs.h>
#include <raylib.h>

#include "camera.h"
#include "input.h"
#include "lighting.h"
#include "shadows.h"
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

    // sync typed light entities -> shader uniforms
    ecs.system("graphics::sync_lights")
        .kind(PreRender)
        .run([](flecs::iter& it) {
            auto* lr = it.world().try_get<LightRenderer>();
            if (!lr || !lr->shader.id) return;

            lr->upload_camera_pos(detail::raylib_camera.position);

            queries::ambient.each([&](const AmbientLight& amb) {
                amb.upload(*lr);
            });

            int idx = 0;
            queries::directional.each([&](const DirectionalLight& dl) {
                if (idx < MAX_LIGHTS) dl.upload(*lr, idx++);
            });
            queries::point.each([&](const PointLight& pl, const Position& pos) {
                if (idx < MAX_LIGHTS) pl.upload(*lr, idx++, pos);
            });
            queries::spot.each([&](const SpotLight& sl, const Position& pos) {
                if (idx < MAX_LIGHTS) sl.upload(*lr, idx++, pos);
            });

            // disable remaining slots
            for (int i = idx; i < MAX_LIGHTS; ++i) {
                int disabled = 0;
                SetShaderValue(lr->shader, lr->light_locs[i].enabled, &disabled, SHADER_UNIFORM_INT);
            }

            lr->upload_light_count(idx);
        });

    // PreRender: begin drawing, clear
    ecs.system("graphics::begin")
        .kind(PreRender)
        .run([](flecs::iter& it) {
            graphics::shim::update();

            if (IsWindowResized()) {
                SetWindowSize(GetScreenWidth(), GetScreenHeight());
            }

            BeginDrawing();
            ClearBackground(graphics::to_color(props::background_color.get<color4f>()));
        });

    // OnRender: begin 3D mode
    ecs.system("graphics::render3d")
        .kind(OnRender)
        .run([](flecs::iter& it) {
            BeginMode3D(detail::raylib_camera);

            if (auto* lr = it.world().try_get<LightRenderer>()) {
                if (lr->shader.id) BeginShaderMode(lr->shader);
            }
        });

    // transition: end PBR shader before overlay draws
    ecs.system("graphics::end_shader")
        .kind(OnRenderOverlay)
        .run([](flecs::iter& it) {
            if (auto* lr = it.world().try_get<LightRenderer>()) {
                if (lr->shader.id) EndShaderMode();
            }
        });

    // PostRender: end 3D mode, draw overlays
    ecs.system("graphics::render2d")
        .kind(PostRender)
        .run([](flecs::iter& it) {
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
