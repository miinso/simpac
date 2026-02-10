#pragma once

#include "../components/core.h"
#include "../components/render.h"
#include "../queries.h"
#include "graphics.h"
#include "render_draw.h"
#include "render_gpu.h"

#include <raylib.h>

#include <limits>

namespace systems {

inline void install_render_systems(flecs::world& ecs) {
    ecs.system<Spring>("graphics::DrawSpringsCPU")
        .kind(graphics::OnRender)
        .each([](Spring& s) {
            render::draw_spring(s);
        }).disable();

    ecs.system<const Position, const Mass>("graphics::DrawParticlesCPU")
        .with<Particle>()
        .kind(graphics::OnRender)
        .each([](const Position& x, const Mass& m) {
            render::draw_particle(x, m);
        }).disable();

    ecs.system("graphics::UploadSpringPositions")
        .kind(graphics::PreRender)
        .run([](flecs::iter& it) {
            auto& ctx = it.world().get_mut<SpringRenderer>();
            render::upload_spring_positions_to_gpu(it.world(), ctx);
        }).disable(0);

    ecs.system("graphics::DrawSpringsGPU")
        .kind(graphics::OnRender)
        .run([](flecs::iter& it) {
            auto& ctx = it.world().get_mut<SpringRenderer>();
            render::draw_springs_gpu(ctx);
        }).disable(0);

    ecs.system("graphics::UploadParticlePositions")
        .kind(graphics::PreRender)
        .run([](flecs::iter& it) {
            auto& ctx = it.world().get_mut<ParticleRenderer>();
            render::upload_particle_positions_to_gpu(it.world(), ctx);
        }).disable(0);

    ecs.system("graphics::DrawParticlesGPU")
        .kind(graphics::OnRender)
        .run([](flecs::iter& it) {
            auto& ctx = it.world().get_mut<ParticleRenderer>();
            render::draw_particles_gpu(ctx);
        }).disable(0);

    ecs.system("graphics::DrawTimingInfo")
        .kind(graphics::PostRender)
        .run(render::draw_timing_info)
        .disable(0);

    ecs.system("graphics::PickParticles")
        .kind(flecs::OnLoad)
        .run([&](flecs::iter& it) {
            auto& pick = it.world().get_mut<MousePickState>();
            auto& renderer = it.world().get<ParticleRenderer>();

            Ray ray = GetMouseRay(GetMousePosition(), graphics::get_raylib_camera_const());
            float pick_radius = renderer.base_radius * pick.pick_radius_scale;

            float closest = std::numeric_limits<float>::max();
            flecs::entity hovered = flecs::entity::null();

            queries::particle_pick_query.each([&](flecs::entity e, const Position& pos, ParticleState&) {
                RayCollision hit = GetRayCollisionSphere(ray, {pos[0], pos[1], pos[2]}, pick_radius);
                if (hit.hit && hit.distance < closest) {
                    closest = hit.distance;
                    hovered = e;
                }
            });

            pick.hovered = hovered;

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                if (hovered.is_alive()) {
                    pick.selected = (pick.selected == hovered)
                        ? flecs::entity::null()
                        : hovered;
                } else {
                    pick.selected = flecs::entity::null();
                }
            }

            queries::particle_pick_query.each([&](flecs::entity e, const Position&, ParticleState& state) {
                constexpr uint32_t kInteractionMask = ParticleState::Hovered | ParticleState::Selected;
                uint32_t flags = (uint32_t)(state.flags & ~kInteractionMask);
                if (e == hovered) flags |= ParticleState::Hovered;
                if (pick.selected == e) flags |= ParticleState::Selected;
                state.flags = flags;
            });
        }).disable(0);
}

} // namespace systems

