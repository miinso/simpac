#pragma once

#include "components.h"
#include "draw.h"
#include "gpu.h"

namespace cloth {

struct render {
    render(flecs::world& ecs) {
        ecs.component<SpringRenderer>()
            .on_set([](flecs::entity e, SpringRenderer& r) {
                init_spring_renderer(e.world(), r);
            })
            .on_remove([](flecs::entity, SpringRenderer& r) {
                shutdown_spring_renderer(r);
            })
            .add(flecs::Singleton);

        ecs.component<ParticleRenderer>()
            .on_set([](flecs::entity e, ParticleRenderer& r) {
                init_particle_renderer(e.world(), r);
            })
            .on_remove([](flecs::entity, ParticleRenderer& r) {
                shutdown_particle_renderer(r);
            })
            .add(flecs::Singleton);

        ecs.component<TriangleRenderer>()
            .on_set([](flecs::entity e, TriangleRenderer& r) {
                init_triangle_renderer(e.world(), r);
            })
            .on_remove([](flecs::entity, TriangleRenderer& r) {
                shutdown_triangle_renderer(r);
            })
            .add(flecs::Singleton);

        ecs.set<SpringRenderer>({});
        ecs.set<ParticleRenderer>({});
        ecs.set<TriangleRenderer>({});

        ecs.system<Spring>("DrawSpringsCPU")
            .kind(graphics::OnRender)
            .each([](Spring& s) { draw::draw_spring(s); })
            .disable();

        ecs.system<const Position, const Mass>("DrawParticlesCPU")
            .with<Particle>()
            .kind(graphics::OnRender)
            .each([](const Position& x, const Mass& m) { draw::draw_particle(x, m); })
            .disable();

        ecs.system("UploadSpringPositions")
            .kind(graphics::PreRender)
            .run([](flecs::iter& it) {
                auto& ctx = it.world().get_mut<SpringRenderer>();
                gpu::upload_spring_positions_to_gpu(it.world(), ctx);
            }).disable(0);

        ecs.system("DrawSpringsGPU")
            .kind(graphics::OnRender)
            .run([](flecs::iter& it) {
                auto& ctx = it.world().get_mut<SpringRenderer>();
                gpu::draw_springs_gpu(ctx);
            }).disable(0);

        ecs.system("UploadParticlePositions")
            .kind(graphics::PreRender)
            .run([](flecs::iter& it) {
                auto& ctx = it.world().get_mut<ParticleRenderer>();
                gpu::upload_particle_positions_to_gpu(it.world(), ctx);
            }).disable(0);

        ecs.system("DrawParticlesGPU")
            .kind(graphics::OnRender)
            .run([](flecs::iter& it) {
                auto& ctx = it.world().get_mut<ParticleRenderer>();
                gpu::draw_particles_gpu(ctx);
            }).disable(0);

        ecs.system("UploadTrianglePositions")
            .kind(graphics::PreRender)
            .run([](flecs::iter& it) {
                auto& ctx = it.world().get_mut<TriangleRenderer>();
                gpu::upload_triangle_positions_to_gpu(it.world(), ctx);
            }).disable(0);

        ecs.system("DrawTrianglesGPU")
            .kind(graphics::OnRender)
            .run([](flecs::iter& it) {
                auto& ctx = it.world().get_mut<TriangleRenderer>();
                gpu::draw_triangles_gpu(ctx);
            }).disable(0);

        ecs.system("DrawTimingInfo")
            .kind(graphics::PostRender)
            .run(draw::draw_timing_info)
            .disable(0);
    }
};

} // namespace cloth
