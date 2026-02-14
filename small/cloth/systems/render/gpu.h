#pragma once

#include "../../components.h"
#include "../../queries.h"
#include "graphics.h"

#include <raylib.h>
#include <rlgl.h>
#include <raymath.h>

#if defined(__EMSCRIPTEN__)
    #include <GLES3/gl3.h>
#elif defined(__APPLE__)
    #include "external/glad_gles2.h"
#else
    #include "external/glad.h"
#endif

#include <vector>

namespace render {
namespace detail {

inline void draw_arrays_instanced(GLenum mode, GLint first, GLsizei count, GLsizei instances) {
#if defined(__APPLE__) && !defined(__EMSCRIPTEN__)
    glDrawArraysInstancedEXT(mode, first, count, instances);
#else
    glDrawArraysInstanced(mode, first, count, instances);
#endif
}

inline void draw_elements_instanced(GLenum mode, GLsizei count, GLenum type, const void* indices, GLsizei instances) {
#if defined(__APPLE__) && !defined(__EMSCRIPTEN__)
    glDrawElementsInstancedEXT(mode, count, type, indices, instances);
#else
    glDrawElementsInstanced(mode, count, type, indices, instances);
#endif
}

inline void pack_spring_instance(std::vector<float>& staging_buffer, int& inst_idx,
                                 const vec3f& a, const vec3f& b, float rest_length) {
    staging_buffer[inst_idx++] = a.x;
    staging_buffer[inst_idx++] = a.y;
    staging_buffer[inst_idx++] = a.z;
    staging_buffer[inst_idx++] = b.x;
    staging_buffer[inst_idx++] = b.y;
    staging_buffer[inst_idx++] = b.z;
    staging_buffer[inst_idx++] = rest_length;
}

inline void pack_particle_instance(std::vector<float>& staging_buffer, int offset,
                                   const vec3f& p, float mass, float flags) {
    staging_buffer[offset + 0] = p.x;
    staging_buffer[offset + 1] = p.y;
    staging_buffer[offset + 2] = p.z;
    staging_buffer[offset + 3] = mass;
    staging_buffer[offset + 4] = flags;
}

} // namespace detail

inline void upload_spring_positions_to_gpu(const flecs::world&, SpringRenderer& gpu) {
    if (gpu.shader_id == 0) return;

    int num_particles = queries::num_particles();
    int num_springs = queries::num_springs();

    if (gpu.allocated_springs != num_springs) {
        gpu.allocated_springs = num_springs;
        gpu.staging_buffer.resize(num_springs * SpringRenderer::FLOATS_PER_INSTANCE);

        if (gpu.vao) rlUnloadVertexArray(gpu.vao);
        if (gpu.instance_vbo) rlUnloadVertexBuffer(gpu.instance_vbo);

        gpu.vao = rlLoadVertexArray();
        rlEnableVertexArray(gpu.vao);
        gpu.instance_vbo = rlLoadVertexBuffer(nullptr, gpu.staging_buffer.size() * sizeof(float), true);

        constexpr int stride = SpringRenderer::FLOATS_PER_INSTANCE * sizeof(float);
        rlEnableVertexAttribute(0);
        rlSetVertexAttribute(0, 3, RL_FLOAT, false, stride, 0);
        rlSetVertexAttributeDivisor(0, 1);
        rlEnableVertexAttribute(1);
        rlSetVertexAttribute(1, 3, RL_FLOAT, false, stride, 3 * sizeof(float));
        rlSetVertexAttributeDivisor(1, 1);
        rlEnableVertexAttribute(2);
        rlSetVertexAttribute(2, 1, RL_FLOAT, false, stride, 6 * sizeof(float));
        rlSetVertexAttributeDivisor(2, 1);

        rlDisableVertexArray();
    }

    if (num_springs == 0) return;

    gpu.spring_particle_indices.clear();
    gpu.spring_particle_indices.reserve(num_springs * 2);
    gpu.rest_lengths.clear();
    gpu.rest_lengths.reserve(num_springs);

    queries::spring_query.each([&](const Spring& s) {
        gpu.spring_particle_indices.push_back(s.e1.get<ParticleIndex>());
        gpu.spring_particle_indices.push_back(s.e2.get<ParticleIndex>());
        gpu.rest_lengths.push_back((float)s.rest_length);
    });

    std::vector<vec3f> positions(num_particles);
    gpu.position_query.run([&](flecs::iter& it) {
        while (it.next()) {
            auto pos = it.field<const Position>(0);
            auto idx = it.field<const ParticleIndex>(1);
            for (auto i : it) {
                positions[idx[i]] = vec3f(pos[i].map());
            }
        }
    });

    int inst_idx = 0;
    for (int i = 0; i < num_springs; ++i) {
        const int idx_a = gpu.spring_particle_indices[i * 2];
        const int idx_b = gpu.spring_particle_indices[i * 2 + 1];
        detail::pack_spring_instance(
            gpu.staging_buffer, inst_idx, positions[idx_a], positions[idx_b], gpu.rest_lengths[i]
        );
    }

    rlUpdateVertexBuffer(gpu.instance_vbo, gpu.staging_buffer.data(),
                         gpu.staging_buffer.size() * sizeof(float), 0);
}

inline void draw_springs_gpu(const SpringRenderer& gpu) {
    if (gpu.shader_id == 0) return;

    Matrix viewproj = MatrixMultiply(rlGetMatrixModelview(), rlGetMatrixProjection());
    rlEnableShader(gpu.shader_id);
    rlSetUniformMatrix(gpu.u_viewproj_loc, viewproj);
    rlSetUniform(gpu.u_strain_scale_loc, &gpu.strain_scale, SHADER_UNIFORM_FLOAT, 1);

    rlEnableVertexArray(gpu.vao);
    rlEnableVertexBuffer(gpu.instance_vbo);
    detail::draw_arrays_instanced(GL_LINES, 0, SpringRenderer::VERTICES_PER_INSTANCE, gpu.allocated_springs);
    rlDisableVertexBuffer();
    rlDisableVertexArray();
    rlDisableShader();
}

inline void upload_particle_positions_to_gpu(const flecs::world& ecs, ParticleRenderer& gpu) {
    if (gpu.shader_id == 0) return;

    int num_particles = queries::num_particles();

    if (gpu.allocated_particles != num_particles) {
        gpu.allocated_particles = num_particles;
        gpu.staging_buffer.resize(num_particles * ParticleRenderer::FLOATS_PER_INSTANCE);

        if (gpu.instance_vbo) rlUnloadVertexBuffer(gpu.instance_vbo);

        rlEnableVertexArray(gpu.vao);
        gpu.instance_vbo = rlLoadVertexBuffer(nullptr, gpu.staging_buffer.size() * sizeof(float), true);

        constexpr int stride = ParticleRenderer::FLOATS_PER_INSTANCE * sizeof(float);
        rlEnableVertexAttribute(2);
        rlSetVertexAttribute(2, 3, RL_FLOAT, false, stride, 0);
        rlSetVertexAttributeDivisor(2, 1);
        rlEnableVertexAttribute(3);
        rlSetVertexAttribute(3, 1, RL_FLOAT, false, stride, 3 * sizeof(float));
        rlSetVertexAttributeDivisor(3, 1);
        rlEnableVertexAttribute(4);
        rlSetVertexAttribute(4, 1, RL_FLOAT, false, stride, 4 * sizeof(float));
        rlSetVertexAttributeDivisor(4, 1);

        rlDisableVertexArray();
    }

    if (num_particles == 0) return;

    gpu.position_query.run([&](flecs::iter& it) {
        while (it.next()) {
            auto pos = it.field<const Position>(0);
            auto idx = it.field<const ParticleIndex>(1);
            auto mass = it.field<const Mass>(2);
            bool has_state = it.is_set(3);
            auto state = it.field<const ParticleState>(3);
            for (auto i : it) {
                int pidx = idx[i];
                if (pidx >= 0 && pidx < num_particles) {
                    const int offset = pidx * ParticleRenderer::FLOATS_PER_INSTANCE;
                    detail::pack_particle_instance(
                        gpu.staging_buffer,
                        offset,
                        vec3f(pos[i].map()),
                        (float)mass[i],
                        has_state ? (float)state[i].flags : 0.0f
                    );
                }
            }
        }
    });

    rlUpdateVertexBuffer(gpu.instance_vbo, gpu.staging_buffer.data(),
                         gpu.staging_buffer.size() * sizeof(float), 0);
}

inline void draw_particles_gpu(const ParticleRenderer& gpu) {
    if (gpu.shader_id == 0 || gpu.num_indices == 0) return;

    Matrix viewproj = MatrixMultiply(rlGetMatrixModelview(), rlGetMatrixProjection());
    rlEnableShader(gpu.shader_id);
    rlSetUniformMatrix(gpu.u_viewproj_loc, viewproj);
    rlSetUniform(gpu.u_base_radius_loc, &gpu.base_radius, SHADER_UNIFORM_FLOAT, 1);
    rlSetUniform(gpu.u_color_loc, gpu.color, SHADER_UNIFORM_VEC3, 1);

    rlEnableVertexArray(gpu.vao);
    rlEnableVertexBuffer(gpu.mesh_vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gpu.mesh_ebo);
    detail::draw_elements_instanced(GL_TRIANGLES, gpu.num_indices, GL_UNSIGNED_INT, nullptr, gpu.allocated_particles);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    rlDisableVertexBuffer();
    rlDisableVertexArray();
    rlDisableShader();
}

} // namespace render

