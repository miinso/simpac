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

    int inst_idx = 0;
    queries::spring_query.each([&](const Spring& s) {
        const auto& a = s.e1.get<Position>();
        const auto& b = s.e2.get<Position>();
        detail::pack_spring_instance(
            gpu.staging_buffer, inst_idx,
            vec3f(a.map()), vec3f(b.map()), (float)s.rest_length
        );
    });

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

    int pidx = 0;
    gpu.position_query.run([&](flecs::iter& it) {
        while (it.next()) {
            auto pos = it.field<const Position>(0);
            auto mass = it.field<const Mass>(1);
            bool has_state = it.is_set(2);
            auto state = it.field<const ParticleState>(2);
            for (auto i : it) {
                const int offset = pidx * ParticleRenderer::FLOATS_PER_INSTANCE;
                detail::pack_particle_instance(
                    gpu.staging_buffer,
                    offset,
                    vec3f(pos[i].map()),
                    (float)mass[i],
                    has_state ? (float)state[i].flags : 0.0f
                );
                pidx++;
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

inline void upload_triangle_positions_to_gpu(const flecs::world&, TriangleRenderer& gpu) {
    if (gpu.shader_id == 0) return;

    int num_triangles = queries::triangle_query.count();

    if (gpu.allocated_triangles != num_triangles) {
        gpu.allocated_triangles = num_triangles;
        gpu.staging_buffer.resize(num_triangles * TriangleRenderer::FLOATS_PER_INSTANCE);
        gpu.rest_areas.resize(num_triangles, 0.0f);

        if (gpu.vao) rlUnloadVertexArray(gpu.vao);
        if (gpu.instance_vbo) rlUnloadVertexBuffer(gpu.instance_vbo);

        gpu.vao = rlLoadVertexArray();
        rlEnableVertexArray(gpu.vao);
        gpu.instance_vbo = rlLoadVertexBuffer(nullptr, gpu.staging_buffer.size() * sizeof(float), true);

        constexpr int stride = TriangleRenderer::FLOATS_PER_INSTANCE * sizeof(float);
        // a_p0 (location 0)
        rlEnableVertexAttribute(0);
        rlSetVertexAttribute(0, 3, RL_FLOAT, false, stride, 0);
        rlSetVertexAttributeDivisor(0, 1);
        // a_p1 (location 1)
        rlEnableVertexAttribute(1);
        rlSetVertexAttribute(1, 3, RL_FLOAT, false, stride, 3 * sizeof(float));
        rlSetVertexAttributeDivisor(1, 1);
        // a_p2 (location 2)
        rlEnableVertexAttribute(2);
        rlSetVertexAttribute(2, 3, RL_FLOAT, false, stride, 6 * sizeof(float));
        rlSetVertexAttributeDivisor(2, 1);
        // a_rest_area (location 3)
        rlEnableVertexAttribute(3);
        rlSetVertexAttribute(3, 1, RL_FLOAT, false, stride, 9 * sizeof(float));
        rlSetVertexAttributeDivisor(3, 1);

        rlDisableVertexArray();

        // cache rest areas on first allocation
        int tidx = 0;
        queries::triangle_query.each([&](const Triangle& tri) {
            const auto p0 = tri.e1.get<Position>().map();
            const auto p1 = tri.e2.get<Position>().map();
            const auto p2 = tri.e3.get<Position>().map();
            Eigen::Vector3r e1 = p1 - p0;
            Eigen::Vector3r e2 = p2 - p0;
            gpu.rest_areas[tidx] = 0.5f * (float)e1.cross(e2).norm();
            tidx++;
        });
    }

    if (num_triangles == 0) return;

    int inst = 0;
    int tidx = 0;
    queries::triangle_query.each([&](const Triangle& tri) {
        const auto& p0 = tri.e1.get<Position>();
        const auto& p1 = tri.e2.get<Position>();
        const auto& p2 = tri.e3.get<Position>();
        gpu.staging_buffer[inst++] = p0.x;
        gpu.staging_buffer[inst++] = p0.y;
        gpu.staging_buffer[inst++] = p0.z;
        gpu.staging_buffer[inst++] = p1.x;
        gpu.staging_buffer[inst++] = p1.y;
        gpu.staging_buffer[inst++] = p1.z;
        gpu.staging_buffer[inst++] = p2.x;
        gpu.staging_buffer[inst++] = p2.y;
        gpu.staging_buffer[inst++] = p2.z;
        gpu.staging_buffer[inst++] = gpu.rest_areas[tidx];
        tidx++;
    });

    rlUpdateVertexBuffer(gpu.instance_vbo, gpu.staging_buffer.data(),
                         gpu.staging_buffer.size() * sizeof(float), 0);
}

inline void draw_triangles_gpu(const TriangleRenderer& gpu) {
    if (gpu.shader_id == 0 || gpu.allocated_triangles == 0) return;

    Matrix viewproj = MatrixMultiply(rlGetMatrixModelview(), rlGetMatrixProjection());
    rlEnableShader(gpu.shader_id);
    rlSetUniformMatrix(gpu.u_viewproj_loc, viewproj);
    rlSetUniform(gpu.u_strain_scale_loc, &gpu.strain_scale, SHADER_UNIFORM_FLOAT, 1);

    glDisable(GL_CULL_FACE);
    rlEnableVertexArray(gpu.vao);
    rlEnableVertexBuffer(gpu.instance_vbo);

    // filled (offset slightly behind wireframe to prevent z-fighting)
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.0f, 1.0f);
    detail::draw_arrays_instanced(GL_TRIANGLES, 0, 3, gpu.allocated_triangles);
    glDisable(GL_POLYGON_OFFSET_FILL);

    // wireframe (GL_LINES, 6 verts per instance = 3 edges, GLES compatible)
    if (gpu.show_wireframe && gpu.wf_shader_id) {
        rlDisableShader();
        rlEnableShader(gpu.wf_shader_id);
        Matrix viewproj2 = MatrixMultiply(rlGetMatrixModelview(), rlGetMatrixProjection());
        rlSetUniformMatrix(gpu.wf_viewproj_loc, viewproj2);
        rlSetUniform(gpu.wf_color_loc, gpu.wireframe_color, SHADER_UNIFORM_VEC3, 1);
        detail::draw_arrays_instanced(GL_LINES, 0, 6, gpu.allocated_triangles);
    }

    rlDisableVertexBuffer();
    rlDisableVertexArray();
    glEnable(GL_CULL_FACE);
    rlDisableShader();
}

} // namespace render
