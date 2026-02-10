#pragma once

#include "../queries.h"
#include "graphics.h"
#include "../systems/render_gpu.h"

#include <raylib.h>

#include <vector>

namespace components {

inline void register_render_components(flecs::world& ecs) {
    ecs.component<MousePickState>()
        .add(flecs::Singleton);

    ecs.component<SpringRenderer>()
        .on_set([](flecs::entity e, SpringRenderer& gpu) {
            auto world = e.world();
            gpu.position_query = world.query_builder<const Position, const ParticleIndex>()
                .cached()
                .build();
            Shader shader = LoadShader(
                graphics::npath("resources/shaders/glsl300es/spring.vs").c_str(),
                graphics::npath("resources/shaders/glsl300es/spring.fs").c_str()
            );
            if (IsShaderValid(shader)) {
                gpu.shader_id = shader.id;
                gpu.u_viewproj_loc = GetShaderLocation(shader, "u_viewproj");
                gpu.u_strain_scale_loc = GetShaderLocation(shader, "u_strain_scale");
            }
        })
        .on_remove([](flecs::entity e, SpringRenderer& gpu) {
            if (gpu.shader_id) UnloadShader({gpu.shader_id});
            if (gpu.instance_vbo) rlUnloadVertexBuffer(gpu.instance_vbo);
            if (gpu.vao) rlUnloadVertexArray(gpu.vao);
        })
        .add(flecs::Singleton);

    ecs.component<ParticleRenderer>()
        .on_set([](flecs::entity e, ParticleRenderer& gpu) {
            auto world = e.world();
            gpu.position_query = world.query_builder<const Position, const ParticleIndex, const ParticleState>()
                .with<Particle>()
                .term_at<ParticleState>().optional()
                .cached()
                .build();

            std::vector<float> vertices;
            std::vector<unsigned int> indices;
            render::generate_icosphere(vertices, indices, 2);
            gpu.num_vertices = vertices.size() / 6;
            gpu.num_indices = indices.size();

            gpu.vao = rlLoadVertexArray();
            rlEnableVertexArray(gpu.vao);

            gpu.mesh_vbo = rlLoadVertexBuffer(vertices.data(), vertices.size() * sizeof(float), false);

            glGenBuffers(1, &gpu.mesh_ebo);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gpu.mesh_ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

            constexpr int stride = 6 * sizeof(float);
            rlEnableVertexAttribute(0);
            rlSetVertexAttribute(0, 3, RL_FLOAT, false, stride, 0);
            rlEnableVertexAttribute(1);
            rlSetVertexAttribute(1, 3, RL_FLOAT, false, stride, 3 * sizeof(float));

            rlDisableVertexArray();

            Shader shader = LoadShader(
                graphics::npath("resources/shaders/glsl300es/particle.vs").c_str(),
                graphics::npath("resources/shaders/glsl300es/particle.fs").c_str()
            );
            if (IsShaderValid(shader)) {
                gpu.shader_id = shader.id;
                gpu.u_viewproj_loc = GetShaderLocation(shader, "u_viewproj");
                gpu.u_color_loc = GetShaderLocation(shader, "u_color");
            }
        })
        .on_remove([](flecs::entity e, ParticleRenderer& gpu) {
            if (gpu.shader_id) UnloadShader({gpu.shader_id});
            if (gpu.instance_vbo) rlUnloadVertexBuffer(gpu.instance_vbo);
            if (gpu.mesh_ebo) glDeleteBuffers(1, &gpu.mesh_ebo);
            if (gpu.mesh_vbo) rlUnloadVertexBuffer(gpu.mesh_vbo);
            if (gpu.vao) rlUnloadVertexArray(gpu.vao);
        })
        .add(flecs::Singleton);
}

} // namespace components

