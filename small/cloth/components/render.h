#pragma once

#include "core.h"

#include <vector>

struct MousePickState {
    flecs::entity hovered;
    flecs::entity selected;
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

namespace components {
void register_render_components(flecs::world& ecs);
} // namespace components

#include "render_install.h"
