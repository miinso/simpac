#pragma once

#include "core.h"

#include <vector>

struct ParticleInteractionState {
    enum DragMode {
        Kinematic = 0,
        VirtualSpring = 1,
    };

    // NOTE: for now we store only single entity but this can change
    flecs::entity hovered;
    flecs::entity selected;
    flecs::entity pressed;

    // persistent, true while left mouse button is currently held
    bool pointer_down = false;
    // instant, true only on the frame the button transitions up -> down
    bool pointer_pressed = false;
    // instant, true only on the frame the button transitions down -> up
    bool pointer_released = false;
    bool dragging = false;

    float press_x = 0.0f;
    float press_y = 0.0f;
    float last_x = 0.0f;
    float last_y = 0.0f;

    vec3f drag_plane_point = {0.0f, 0.0f, 0.0f};
    vec3f drag_plane_normal = {0.0f, 1.0f, 0.0f};
    vec3f drag_offset = {0.0f, 0.0f, 0.0f};
    bool drag_added_pin = false;

    int drag_mode = VirtualSpring;
    float virtual_spring_k = 800.0f;
    float virtual_spring_d = 24.0f;

    float drag_threshold_px = 3.0f;
    float pick_radius_scale = 1.2f;
};

struct SpringRenderer {
    static constexpr int FLOATS_PER_INSTANCE = 7;
    static constexpr int VERTICES_PER_INSTANCE = 2;

    unsigned int vao = 0;
    unsigned int instance_vbo = 0;
    unsigned int shader_id = 0;

    int u_viewproj_loc = -1;
    int u_strain_scale_loc = -1;

    float strain_scale = 10.0f;
    int allocated_springs = 0;

    flecs::query<const Position, const ParticleIndex> position_query;

    std::vector<float> staging_buffer;
    std::vector<int> spring_particle_indices;
    std::vector<float> rest_lengths;
};

struct ParticleRenderer {
    static constexpr int FLOATS_PER_INSTANCE = 5;

    unsigned int vao = 0;
    unsigned int mesh_vbo = 0;
    unsigned int mesh_ebo = 0;
    unsigned int instance_vbo = 0;
    unsigned int shader_id = 0;

    int num_vertices = 0;
    int num_indices = 0;

    int u_viewproj_loc = -1;
    int u_base_radius_loc = -1;
    int u_color_loc = -1;

    float base_radius = 0.5f;
    float color[3] = {0.2f, 0.5f, 0.9f};

    int allocated_particles = 0;

    flecs::query<const Position, const ParticleIndex, const Mass, const ParticleState> position_query;
    std::vector<float> staging_buffer;
};

#include "graphics.h"

#include <raylib.h>
#include <rlgl.h>

#if defined(__EMSCRIPTEN__)
    #include <GLES3/gl3.h>
#elif defined(__APPLE__)
    #include "external/glad_gles2.h"
#else
    #include "external/glad.h"
#endif

#include "par_shapes.h"

namespace render {

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

} // namespace render

namespace components {

inline void register_render_components(flecs::world& ecs) {
    ecs.component<ParticleInteractionState>()
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
            gpu.position_query = world.query_builder<const Position, const ParticleIndex, const Mass, const ParticleState>()
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
                gpu.u_base_radius_loc = GetShaderLocation(shader, "u_base_radius");
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
