#pragma once

#include "core.h"
#include "../components/render.h"
#include "../render/draw.h"
#include "../render/gpu.h"

namespace cloth {

struct render {
    render(flecs::world& ecs) {
        ecs.component<SpringRenderer>()
            .on_set([](flecs::entity e, SpringRenderer& gpu) {
                ::render::detail::init_spring_renderer(e.world(), gpu);
            })
            .on_remove([](flecs::entity, SpringRenderer& gpu) {
                ::render::detail::shutdown_spring_renderer(gpu);
            })
            .add(flecs::Singleton);

        ecs.component<ParticleRenderer>()
            .on_set([](flecs::entity e, ParticleRenderer& gpu) {
                ::render::detail::init_particle_renderer(e.world(), gpu);
            })
            .on_remove([](flecs::entity, ParticleRenderer& gpu) {
                ::render::detail::shutdown_particle_renderer(gpu);
            })
            .add(flecs::Singleton);

        ecs.component<TriangleRenderer>()
            .on_set([](flecs::entity e, TriangleRenderer& gpu) {
                ::render::detail::init_triangle_renderer(e.world(), gpu);
            })
            .on_remove([](flecs::entity, TriangleRenderer& gpu) {
                ::render::detail::shutdown_triangle_renderer(gpu);
            })
            .add(flecs::Singleton);

        ecs.set<SpringRenderer>({});
        ecs.set<ParticleRenderer>({});
        ecs.set<TriangleRenderer>({});

        ecs.system<Spring>("DrawSpringsCPU")
            .kind(graphics::OnRender)
            .each([](Spring& s) { ::render::draw_spring(s); })
            .disable();

        ecs.system<const Position, const Mass>("DrawParticlesCPU")
            .with<Particle>()
            .kind(graphics::OnRender)
            .each([](const Position& x, const Mass& m) { ::render::draw_particle(x, m); })
            .disable();

        ecs.system("UploadSpringPositions")
            .kind(graphics::PreRender)
            .run([](flecs::iter& it) {
                auto& ctx = it.world().get_mut<SpringRenderer>();
                ::render::upload_spring_positions_to_gpu(it.world(), ctx);
            }).disable(0);

        ecs.system("DrawSpringsGPU")
            .kind(graphics::OnRender)
            .run([](flecs::iter& it) {
                auto& ctx = it.world().get_mut<SpringRenderer>();
                ::render::draw_springs_gpu(ctx);
            }).disable(0);

        ecs.system("UploadParticlePositions")
            .kind(graphics::PreRender)
            .run([](flecs::iter& it) {
                auto& ctx = it.world().get_mut<ParticleRenderer>();
                ::render::upload_particle_positions_to_gpu(it.world(), ctx);
            }).disable(0);

        ecs.system("DrawParticlesGPU")
            .kind(graphics::OnRender)
            .run([](flecs::iter& it) {
                auto& ctx = it.world().get_mut<ParticleRenderer>();
                ::render::draw_particles_gpu(ctx);
            }).disable(0);

        ecs.system("UploadTrianglePositions")
            .kind(graphics::PreRender)
            .run([](flecs::iter& it) {
                auto& ctx = it.world().get_mut<TriangleRenderer>();
                ::render::upload_triangle_positions_to_gpu(it.world(), ctx);
            }).disable(0);

        ecs.system("DrawTrianglesGPU")
            .kind(graphics::OnRender)
            .run([](flecs::iter& it) {
                auto& ctx = it.world().get_mut<TriangleRenderer>();
                ::render::draw_triangles_gpu(ctx);
            }).disable(0);

        ecs.system("DrawTimingInfo")
            .kind(graphics::PostRender)
            .run(::render::draw_timing_info)
            .disable(0);
    }
};

} // namespace cloth
