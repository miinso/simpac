#pragma once

#include "../components.h"
#include "../queries.h"
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

#include "par_shapes.h"

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

} // namespace detail

inline void generate_icosphere(std::vector<float>& vertices, std::vector<unsigned int>& indices, int subdivisions = 1) {
    par_shapes_mesh* mesh = par_shapes_create_subdivided_sphere(subdivisions);

    vertices.clear();
    vertices.reserve(mesh->npoints * 6);

    for (int i = 0; i < mesh->npoints; ++i) {
        vertices.push_back(mesh->points[i * 3 + 0]);
        vertices.push_back(mesh->points[i * 3 + 1]);
        vertices.push_back(mesh->points[i * 3 + 2]);

        if (mesh->normals) {
            vertices.push_back(mesh->normals[i * 3 + 0]);
            vertices.push_back(mesh->normals[i * 3 + 1]);
            vertices.push_back(mesh->normals[i * 3 + 2]);
        } else {
            vertices.push_back(mesh->points[i * 3 + 0]);
            vertices.push_back(mesh->points[i * 3 + 1]);
            vertices.push_back(mesh->points[i * 3 + 2]);
        }
    }

    indices.clear();
    indices.reserve(mesh->ntriangles * 3);
    for (int i = 0; i < mesh->ntriangles * 3; ++i) {
        indices.push_back(mesh->triangles[i]);
    }

    par_shapes_free_mesh(mesh);
}

inline void upload_spring_positions_to_gpu(const flecs::world& ecs, SpringRenderer& gpu) {
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

    std::vector<Eigen::Vector3r> positions(num_particles);
    gpu.position_query.run([&](flecs::iter& it) {
        while (it.next()) {
            auto pos = it.field<const Position>(0);
            auto idx = it.field<const ParticleIndex>(1);
            for (auto i : it) {
                positions[idx[i]] = pos[i].map();
            }
        }
    });

    int inst_idx = 0;
    for (int i = 0; i < num_springs; ++i) {
        int idx_a = gpu.spring_particle_indices[i * 2];
        int idx_b = gpu.spring_particle_indices[i * 2 + 1];
        Eigen::Vector3r pa = positions[idx_a];
        Eigen::Vector3r pb = positions[idx_b];
        float rest = gpu.rest_lengths[i];

        gpu.staging_buffer[inst_idx++] = (float)pa.x();
        gpu.staging_buffer[inst_idx++] = (float)pa.y();
        gpu.staging_buffer[inst_idx++] = (float)pa.z();
        gpu.staging_buffer[inst_idx++] = (float)pb.x();
        gpu.staging_buffer[inst_idx++] = (float)pb.y();
        gpu.staging_buffer[inst_idx++] = (float)pb.z();
        gpu.staging_buffer[inst_idx++] = rest;
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
            bool has_state = it.is_set(2);
            auto state = it.field<const ParticleState>(2);
            for (auto i : it) {
                int pidx = idx[i];
                if (pidx >= 0 && pidx < num_particles) {
                    int offset = pidx * ParticleRenderer::FLOATS_PER_INSTANCE;
                    gpu.staging_buffer[offset + 0] = pos[i].x;
                    gpu.staging_buffer[offset + 1] = pos[i].y;
                    gpu.staging_buffer[offset + 2] = pos[i].z;
                    gpu.staging_buffer[offset + 3] = gpu.base_radius;
                    gpu.staging_buffer[offset + 4] = has_state ? (float)state[i].flags : 0.0f;
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

