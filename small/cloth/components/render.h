#pragma once

#include "particle.h"

#include <vector>

struct ParticleInteractionState {
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

    float virtual_spring_k = 800.0f;
    float virtual_spring_d = 24.0f;

    float drag_threshold_px = 3.0f;
    float pick_radius_scale = 1.2f;
};

struct TriangleInteractionState {
    flecs::entity hovered;
    flecs::entity selected;
    flecs::entity pressed;

    bool pointer_down = false;
    bool pointer_pressed = false;
    bool pointer_released = false;
    bool dragging = false;

    float press_x = 0.0f;
    float press_y = 0.0f;
    float last_x = 0.0f;
    float last_y = 0.0f;

    vec3f drag_plane_point = {0.0f, 0.0f, 0.0f};
    vec3f drag_plane_normal = {0.0f, 1.0f, 0.0f};
    vec3f drag_offset = {0.0f, 0.0f, 0.0f};

    // vertex offsets from centroid at press time
    vec3f drag_vertex_offsets[3] = {};
    bool drag_added_pins = false;

    float virtual_spring_k = 800.0f;
    float virtual_spring_d = 24.0f;
    float drag_threshold_px = 3.0f;
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

    std::vector<float> staging_buffer;
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

    float base_radius = 0.15f;
    float color[3] = {0.2f, 0.5f, 0.9f};

    int allocated_particles = 0;

    flecs::query<const Position, const Mass, const ParticleState> position_query;
    std::vector<float> staging_buffer;
};

struct TriangleRenderer {
    static constexpr int FLOATS_PER_INSTANCE = 11; // p0(3) + p1(3) + p2(3) + rest_area(1) + flags(1)
    static constexpr int VERTICES_PER_INSTANCE = 3;

    unsigned int vao = 0;
    unsigned int instance_vbo = 0;

    // filled shader
    unsigned int shader_id = 0;
    int u_viewproj_loc = -1;
    int u_strain_scale_loc = -1;
    float strain_scale = 5.0f;

    // wireframe shader (same vao/vbo, 6 verts per instance as GL_LINES)
    unsigned int wf_shader_id = 0;
    int wf_viewproj_loc = -1;
    int wf_color_loc = -1;
    float wireframe_color[3] = {0.15f, 0.15f, 0.15f};
    bool show_wireframe = true;

    int allocated_triangles = 0;
    std::vector<float> staging_buffer;
    std::vector<float> rest_areas;
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

namespace detail {

inline void init_spring_renderer(flecs::world world, SpringRenderer& gpu) {
    Shader shader = LoadShader(
        graphics::npath("resources/shaders/glsl300es/spring.vs").c_str(),
        graphics::npath("resources/shaders/glsl300es/spring.fs").c_str()
    );
    if (IsShaderValid(shader)) {
        gpu.shader_id = shader.id;
        gpu.u_viewproj_loc = GetShaderLocation(shader, "u_viewproj");
        gpu.u_strain_scale_loc = GetShaderLocation(shader, "u_strain_scale");
    }
}

inline void shutdown_spring_renderer(SpringRenderer& gpu) {
    if (gpu.shader_id) UnloadShader({gpu.shader_id});
    if (gpu.instance_vbo) rlUnloadVertexBuffer(gpu.instance_vbo);
    if (gpu.vao) rlUnloadVertexArray(gpu.vao);
}

inline void init_particle_renderer(flecs::world world, ParticleRenderer& gpu) {
    gpu.position_query = world.query_builder<const Position, const Mass, const ParticleState>()
        .with<Particle>()
        .term_at<ParticleState>().optional()
        .cached()
        .build();

    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    generate_icosphere(vertices, indices, 2);
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
}

inline void shutdown_particle_renderer(ParticleRenderer& gpu) {
    if (gpu.shader_id) UnloadShader({gpu.shader_id});
    if (gpu.instance_vbo) rlUnloadVertexBuffer(gpu.instance_vbo);
    if (gpu.mesh_ebo) glDeleteBuffers(1, &gpu.mesh_ebo);
    if (gpu.mesh_vbo) rlUnloadVertexBuffer(gpu.mesh_vbo);
    if (gpu.vao) rlUnloadVertexArray(gpu.vao);
}

inline void init_triangle_renderer(flecs::world world, TriangleRenderer& gpu) {
    Shader shader = LoadShader(
        graphics::npath("resources/shaders/glsl300es/triangle.vs").c_str(),
        graphics::npath("resources/shaders/glsl300es/triangle.fs").c_str()
    );
    if (IsShaderValid(shader)) {
        gpu.shader_id = shader.id;
        gpu.u_viewproj_loc = GetShaderLocation(shader, "u_viewproj");
        gpu.u_strain_scale_loc = GetShaderLocation(shader, "u_strain_scale");
    }

    Shader wf = LoadShader(
        graphics::npath("resources/shaders/glsl300es/triangle_wireframe.vs").c_str(),
        graphics::npath("resources/shaders/glsl300es/triangle_wireframe.fs").c_str()
    );
    if (IsShaderValid(wf)) {
        gpu.wf_shader_id = wf.id;
        gpu.wf_viewproj_loc = GetShaderLocation(wf, "u_viewproj");
        gpu.wf_color_loc = GetShaderLocation(wf, "u_color");
    }
}

inline void shutdown_triangle_renderer(TriangleRenderer& gpu) {
    if (gpu.shader_id) UnloadShader({gpu.shader_id});
    if (gpu.wf_shader_id) UnloadShader({gpu.wf_shader_id});
    if (gpu.instance_vbo) rlUnloadVertexBuffer(gpu.instance_vbo);
    if (gpu.vao) rlUnloadVertexArray(gpu.vao);
}

} // namespace detail

} // namespace render

namespace components {

inline void register_render_components(flecs::world& ecs) {
    ecs.component<ParticleInteractionState>()
        .add(flecs::Singleton);

    ecs.component<TriangleInteractionState>()
        .add(flecs::Singleton);

    ecs.component<SpringRenderer>()
        .on_set([](flecs::entity e, SpringRenderer& gpu) {
            render::detail::init_spring_renderer(e.world(), gpu);
        })
        .on_remove([](flecs::entity, SpringRenderer& gpu) {
            render::detail::shutdown_spring_renderer(gpu);
        })
        .add(flecs::Singleton);

    ecs.component<ParticleRenderer>()
        .on_set([](flecs::entity e, ParticleRenderer& gpu) {
            render::detail::init_particle_renderer(e.world(), gpu);
        })
        .on_remove([](flecs::entity, ParticleRenderer& gpu) {
            render::detail::shutdown_particle_renderer(gpu);
        })
        .add(flecs::Singleton);

    ecs.component<TriangleRenderer>()
        .on_set([](flecs::entity e, TriangleRenderer& gpu) {
            render::detail::init_triangle_renderer(e.world(), gpu);
        })
        .on_remove([](flecs::entity, TriangleRenderer& gpu) {
            render::detail::shutdown_triangle_renderer(gpu);
        })
        .add(flecs::Singleton);
}

} // namespace components
