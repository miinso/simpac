#pragma once

#include "../components/render.h"
#include "graphics.h"
#include "draw.h"
#include "gpu.h"
#include "interaction.h"

namespace systems {

// TODO: do module-centric directories
// like:
//      render/components
//      render/systems
// not like:
//      components/render
//      systems/render

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

    ecs.system("graphics::UploadTrianglePositions")
        .kind(graphics::PreRender)
        .run([](flecs::iter& it) {
            auto& ctx = it.world().get_mut<TriangleRenderer>();
            render::upload_triangle_positions_to_gpu(it.world(), ctx);
        }).disable(0);

    ecs.system("graphics::DrawTrianglesGPU")
        .kind(graphics::OnRender)
        .run([](flecs::iter& it) {
            auto& ctx = it.world().get_mut<TriangleRenderer>();
            render::draw_triangles_gpu(ctx);
        }).disable(0);

    ecs.system("graphics::DrawTimingInfo")
        .kind(graphics::PostRender)
        .run(render::draw_timing_info)
        .disable(0);

    ecs.system("graphics::PickParticles")
        .kind(flecs::OnLoad)
        .run(interaction::pick_particles)
        .disable(0);

    ecs.system("graphics::DragParticlesKinematic")
        .kind(graphics::PreRender)
        .run(interaction::drag_particles_kinematic)
        .disable(0);

    ecs.system("graphics::DragParticlesSpring")
        .kind(graphics::PreRender)
        .run(interaction::drag_particles_spring)
        .disable();

    ecs.system("graphics::DrawDragPlane")
        .kind(graphics::OnRender)
        .run(interaction::draw_drag_plane_debug)
        .disable(0);
}

} // namespace systems
