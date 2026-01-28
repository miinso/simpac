#pragma once

#include "../components.h"
#include "../queries.h"
#include "graphics.h"
#include "flecs.h"
#include <raylib.h>
#include <rlgl.h>
#include <raymath.h>

#if defined(__EMSCRIPTEN__)
    #include <GLES3/gl3.h> // for browser
#elif defined(__APPLE__)
    #include "external/glad_gles2.h"
#else
    #include "external/glad.h"
#endif

#include <vector>
#include "par_shapes.h"

namespace systems {
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

// =========================================================================
// CPU Drawing (not actually CPU per se, it's just simple debug rendering unlike fancy gpu sutff)
// =========================================================================

inline void draw_spring(Spring& spring) {
    auto x1 = spring.e1.get<Position>().map();
    auto x2 = spring.e2.get<Position>().map();

    auto diff = x1 - x2;
    auto current_length = diff.norm();
    auto strain = (current_length - spring.rest_length) / spring.rest_length;

    // color based on strain: compression(blue) -> rest(green) -> tension(red)
    Color color;
    if (strain > 0) {
        float t = std::min(1.0f, (float)strain * 10.0f);
        color = ColorLerp(GREEN, RED, t);
    } else {
        float t = std::min(1.0f, (float)(-strain) * 10.0f);
        color = ColorLerp(GREEN, BLUE, t);
    }

    Vector3 a{x1[0], x1[1], x1[2]};
    Vector3 b{x2[0], x2[1], x2[2]};
    // DrawLine3D(a, b, color);
    float base_thickness = 0.005f;
    float thickness = base_thickness * std::pow((float)spring.k_s, 1.0f / 3.0f);
    DrawCylinderEx(a, b, thickness, thickness, 5, color);
}

inline void draw_particle(const Position& x, const Mass& m) {
    Vector3 pos{x[0], x[1], x[2]};
    DrawPoint3D(pos, BLUE);
    DrawSphere(pos, 0.5, BLUE);
}

inline void draw_timing_info(flecs::iter& it) {
    auto& scene = it.world().get_mut<Scene>();
    auto& solver = it.world().get<Solver>();
    scene.wall_time += it.delta_time();

    Font font = graphics::get_font();
    char buf[512];
    snprintf(buf, sizeof(buf),
        "Wall time: %.2fs  |  Sim time: %.2fs  (dt=%.4f)\n"
        "Frame: %d  |  Speed: %.2fx\n"
        "CG: %d/%d iter, tol=%.0e, err=%.2e\n"
        "\n"
        "Particles: %d  Springs: %d\n"
        "\n"
        "Strain: Red (tension), Green (rest), Blue (compression)\n"
        "Camera: WASDQE / Arrows / MouseDrag / MouseScroll",
        scene.wall_time, scene.sim_time, scene.dt,
        scene.frame_count, scene.sim_time / (scene.wall_time + 1e-9),
        solver.cg_iterations, solver.cg_max_iter, solver.cg_tolerance, solver.cg_error,
        queries::num_particles(), queries::num_springs());
    DrawTextEx(font, buf, {21, 41}, 12, 0, WHITE);
    DrawTextEx(font, buf, {20, 40}, 12, 0, DARKGRAY);

    // draw CG history (right-aligned)
    if (!solver.cg_history.empty()) {
        std::string history_text;
        for (const auto& line : solver.cg_history) {
            history_text += line + "\n";
        }

        int screen_width = GetScreenWidth();
        int screen_height = GetScreenHeight();
        Vector2 text_size = MeasureTextEx(font, history_text.c_str(), 12, 0);
        float x = screen_width - text_size.x - 20;  // 20px padding from right edge
        float y = screen_height - text_size.y;
        DrawTextEx(font, history_text.c_str(), {x, y}, 12, 0, BLACK);  // shadow
        // DrawTextEx(font, history_text.c_str(), {x, 20}, 12, 0, WHITE);
    }
}

// =========================================================================
// GPU Rendering Utilities
// =========================================================================

// generate icosphere mesh using par_shapes
inline void generate_icosphere(std::vector<float>& vertices, std::vector<unsigned int>& indices, int subdivisions = 1) {
    par_shapes_mesh* mesh = par_shapes_create_subdivided_sphere(subdivisions);

    // pack into output format: [x, y, z, nx, ny, nz] per vertex
    vertices.clear();
    vertices.reserve(mesh->npoints * 6);

    for (int i = 0; i < mesh->npoints; ++i) {
        // position
        vertices.push_back(mesh->points[i * 3 + 0]);
        vertices.push_back(mesh->points[i * 3 + 1]);
        vertices.push_back(mesh->points[i * 3 + 2]);

        // normal
        if (mesh->normals) {
            vertices.push_back(mesh->normals[i * 3 + 0]);
            vertices.push_back(mesh->normals[i * 3 + 1]);
            vertices.push_back(mesh->normals[i * 3 + 2]);
        } else {
            // for sphere, normal = normalized position
            vertices.push_back(mesh->points[i * 3 + 0]);
            vertices.push_back(mesh->points[i * 3 + 1]);
            vertices.push_back(mesh->points[i * 3 + 2]);
        }
    }

    // copy indices
    indices.clear();
    indices.reserve(mesh->ntriangles * 3);
    for (int i = 0; i < mesh->ntriangles * 3; ++i) {
        indices.push_back(mesh->triangles[i]);
    }

    par_shapes_free_mesh(mesh);
}

// =========================================================================
// GPU Spring Rendering
// =========================================================================

inline void upload_spring_positions_to_gpu(const flecs::world& ecs, SpringRenderer& gpu) {
    if (gpu.shader_id == 0) return;

    int num_particles = queries::num_particles();
    int num_springs = queries::num_springs();

    // lazy buffer allocation/reallocation on topology change
    if (gpu.allocated_springs != num_springs) {
        gpu.allocated_springs = num_springs;

        // resize staging buffer (7 floats per instance)
        gpu.staging_buffer.resize(num_springs * SpringRenderer::FLOATS_PER_INSTANCE);

        // recreate gpu buffers and configure instanced attributes once
        if (gpu.vao) rlUnloadVertexArray(gpu.vao);
        if (gpu.instance_vbo) rlUnloadVertexBuffer(gpu.instance_vbo);

        gpu.vao = rlLoadVertexArray();
        rlEnableVertexArray(gpu.vao);
        gpu.instance_vbo = rlLoadVertexBuffer(nullptr, gpu.staging_buffer.size() * sizeof(float), true);

        // configure per-instance attributes (done once, stored in vao)
        constexpr int stride = SpringRenderer::FLOATS_PER_INSTANCE * sizeof(float);
        rlEnableVertexAttribute(0);
        rlSetVertexAttribute(0, 3, RL_FLOAT, false, stride, 0);                      // pos_a
        rlSetVertexAttributeDivisor(0, 1);  // instanced
        rlEnableVertexAttribute(1);
        rlSetVertexAttribute(1, 3, RL_FLOAT, false, stride, 3 * sizeof(float));      // pos_b
        rlSetVertexAttributeDivisor(1, 1);  // instanced
        rlEnableVertexAttribute(2);
        rlSetVertexAttribute(2, 1, RL_FLOAT, false, stride, 6 * sizeof(float));      // rest_len
        rlSetVertexAttributeDivisor(2, 1);  // instanced

        rlDisableVertexArray();
    }

    if (num_springs == 0) return;

    // refresh spring connectivity/rest length every frame
    gpu.spring_particle_indices.clear();
    gpu.spring_particle_indices.reserve(num_springs * 2);
    gpu.rest_lengths.clear();
    gpu.rest_lengths.reserve(num_springs);

    queries::spring_query.each([&](const Spring& s) {
        gpu.spring_particle_indices.push_back(s.e1.get<ParticleIndex>());
        gpu.spring_particle_indices.push_back(s.e2.get<ParticleIndex>());
        gpu.rest_lengths.push_back((float)s.rest_length);
    });

    // collect positions using cached query
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

    // build instance buffer (1 instance per spring)
    int inst_idx = 0;
    for (int i = 0; i < num_springs; ++i) {
        int idx_a = gpu.spring_particle_indices[i * 2];
        int idx_b = gpu.spring_particle_indices[i * 2 + 1];
        Eigen::Vector3r pa = positions[idx_a];
        Eigen::Vector3r pb = positions[idx_b];
        float rest = gpu.rest_lengths[i];

        // instance data: [pos_a.xyz, pos_b.xyz, rest_len]
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

    // setup shader and uniforms
    Matrix viewproj = MatrixMultiply(rlGetMatrixModelview(), rlGetMatrixProjection());
    rlEnableShader(gpu.shader_id);
    rlSetUniformMatrix(gpu.u_viewproj_loc, viewproj);
    rlSetUniform(gpu.u_strain_scale_loc, &gpu.strain_scale, SHADER_UNIFORM_FLOAT, 1);

    // bind vao (instanced attributes already configured) and vbo
    rlEnableVertexArray(gpu.vao);
    rlEnableVertexBuffer(gpu.instance_vbo);

    // draw all springs with instanced rendering
    // 2 vertices per instance (line), repeated for each spring
    detail::draw_arrays_instanced(GL_LINES, 0, SpringRenderer::VERTICES_PER_INSTANCE, gpu.allocated_springs);

    // cleanup
    rlDisableVertexBuffer();
    rlDisableVertexArray();
    rlDisableShader();
}

// =========================================================================
// GPU Particle Rendering
// =========================================================================

inline void upload_particle_positions_to_gpu(const flecs::world& ecs, ParticleRenderer& gpu) {
    if (gpu.shader_id == 0) return;

    int num_particles = queries::num_particles();

    // lazy buffer allocation/reallocation on topology change
    if (gpu.allocated_particles != num_particles) {
        gpu.allocated_particles = num_particles;

        // resize staging buffer (4 floats per instance)
        gpu.staging_buffer.resize(num_particles * ParticleRenderer::FLOATS_PER_INSTANCE);

        // recreate instance buffer only (mesh is static)
        if (gpu.instance_vbo) rlUnloadVertexBuffer(gpu.instance_vbo);

        rlEnableVertexArray(gpu.vao);
        gpu.instance_vbo = rlLoadVertexBuffer(nullptr, gpu.staging_buffer.size() * sizeof(float), true);

        // configure per-instance attributes (location 2+)
        constexpr int stride = ParticleRenderer::FLOATS_PER_INSTANCE * sizeof(float);
        rlEnableVertexAttribute(2);
        rlSetVertexAttribute(2, 3, RL_FLOAT, false, stride, 0);  // instance position
        rlSetVertexAttributeDivisor(2, 1);  // instanced
        rlEnableVertexAttribute(3);
        rlSetVertexAttribute(3, 1, RL_FLOAT, false, stride, 3 * sizeof(float));  // instance radius
        rlSetVertexAttributeDivisor(3, 1);  // instanced
        rlEnableVertexAttribute(4);
        rlSetVertexAttribute(4, 1, RL_FLOAT, false, stride, 4 * sizeof(float));  // instance state
        rlSetVertexAttributeDivisor(4, 1);  // instanced

        rlDisableVertexArray();
    }

    if (num_particles == 0) return;

    // collect particle positions
    gpu.position_query.run([&](flecs::iter& it) {
        while (it.next()) {
            auto pos = it.field<const Position>(0);
            auto idx = it.field<const ParticleIndex>(1);
            bool has_state = it.is_set(2); // TODO: all particles should have `ParticleState`
            auto state = it.field<const ParticleState>(2);
            for (auto i : it) {
                int pidx = idx[i];
                if (pidx >= 0 && pidx < num_particles) {
                    int offset = pidx * ParticleRenderer::FLOATS_PER_INSTANCE;
                    gpu.staging_buffer[offset + 0] = pos[i].x;
                    gpu.staging_buffer[offset + 1] = pos[i].y;
                    gpu.staging_buffer[offset + 2] = pos[i].z;
                    gpu.staging_buffer[offset + 3] = gpu.base_radius;
                    gpu.staging_buffer[offset + 4] = has_state
                        ? (float)state[i].flags
                        : 0.0f;
                }
            }
        }
    });

    rlUpdateVertexBuffer(gpu.instance_vbo, gpu.staging_buffer.data(),
                         gpu.staging_buffer.size() * sizeof(float), 0);
}

inline void draw_particles_gpu(const ParticleRenderer& gpu) {
    if (gpu.shader_id == 0 || gpu.num_indices == 0) return;

    // mixing raylib api and raw gl calls just works, i'm impressed..

    // setup shader and uniforms
    Matrix viewproj = MatrixMultiply(rlGetMatrixModelview(), rlGetMatrixProjection());
    rlEnableShader(gpu.shader_id);
    rlSetUniformMatrix(gpu.u_viewproj_loc, viewproj);
    rlSetUniform(gpu.u_color_loc, gpu.color, SHADER_UNIFORM_VEC3, 1);

    // bind vao (mesh + instanced attributes configured) and buffers
    rlEnableVertexArray(gpu.vao);
    rlEnableVertexBuffer(gpu.mesh_vbo);

    // bind element buffer for indexed drawing
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gpu.mesh_ebo);

    // draw instanced icospheres
    detail::draw_elements_instanced(GL_TRIANGLES, gpu.num_indices, GL_UNSIGNED_INT, nullptr, gpu.allocated_particles);

    // cleanup
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    rlDisableVertexBuffer();
    rlDisableVertexArray();
    rlDisableShader();
}

// =========================================================================
// ECS Rendering Setup
// =========================================================================

inline void register_render_components(flecs::world& ecs) {
    ecs.component<InteractionState>()
        .add(flecs::Singleton);

    // SpringRenderer with on_set hook for query and shader initialization
    ecs.component<SpringRenderer>()
        .on_set([](flecs::entity e, SpringRenderer& gpu) {
            auto world = e.world();
            // Cached query for per-frame position lookup
            gpu.position_query = world.query_builder<const Position, const ParticleIndex>()
                .cached()
                .build();
            // Load shader (requires graphics to be initialized first)
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

    // ParticleRenderer with on_set hook for mesh, query, and shader initialization
    ecs.component<ParticleRenderer>()
        .on_set([](flecs::entity e, ParticleRenderer& gpu) {
            auto world = e.world();
            // cached query for per-frame position lookup
            gpu.position_query = world.query_builder<const Position, const ParticleIndex, const ParticleState>()
                .with<Particle>()
                .term_at<ParticleState>().optional() // TODO: necessary?? use maybe prefab to ensure this comp.
                .cached()
                .build();

            // generate icosphere mesh
            std::vector<float> vertices;
            std::vector<unsigned int> indices;
            generate_icosphere(vertices, indices, 2); // last param for subdiv levle
            gpu.num_vertices = vertices.size() / 6;  // 6 floats per vertex (pos+normal)
            gpu.num_indices = indices.size();

            // create vao and upload mesh (static)
            gpu.vao = rlLoadVertexArray();
            rlEnableVertexArray(gpu.vao);

            gpu.mesh_vbo = rlLoadVertexBuffer(vertices.data(), vertices.size() * sizeof(float), false);

            // create element buffer (index buffer)
            glGenBuffers(1, &gpu.mesh_ebo);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gpu.mesh_ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

            // configure mesh attributes (location 0-1)
            constexpr int stride = 6 * sizeof(float);
            rlEnableVertexAttribute(0);
            rlSetVertexAttribute(0, 3, RL_FLOAT, false, stride, 0);  // position
            rlEnableVertexAttribute(1);
            rlSetVertexAttribute(1, 3, RL_FLOAT, false, stride, 3 * sizeof(float));  // normal

            rlDisableVertexArray();

            // load shader
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

inline void install_graphics(flecs::world& ecs) {
    // initialize graphics module
    // (must call `::init()` before any raylib or gl specifics, otherwise crash)
    graphics::init(ecs);
    graphics::init_window(800, 600, "Base Simulator");

    // now set SpringRenderer to trigger on_set hook (after graphics init)
    ecs.set<SpringRenderer>({});

    // set ParticleRenderer to trigger on_set hook
    ecs.set<ParticleRenderer>({});
}

inline void install_render_systems(flecs::world& ecs) {
    // cpu drawing
    ecs.system<Spring>("graphics::DrawSpringsCPU")
        .kind(graphics::phase_on_render)
        .each([](Spring& s) {
            systems::draw_spring(s);
        }).disable();

    ecs.system<const Position, const Mass>("graphics::DrawParticlesCPU")
        .with<Particle>()
        .kind(graphics::phase_on_render)
        .each([](const Position& x, const Mass& m) {
            systems::draw_particle(x, m);
        }).disable();

    // upload positions each frame before rendering
    ecs.system("graphics::UploadSpringPositions")
        .kind(graphics::phase_pre_render)
        .run([](flecs::iter& it) {
            auto& ctx = it.world().get_mut<SpringRenderer>();
            systems::upload_spring_positions_to_gpu(it.world(), ctx);
        }).disable(0);

    ecs.system("graphics::DrawSpringsGPU")
        .kind(graphics::phase_on_render)
        .run([](flecs::iter& it) {
            auto& ctx = it.world().get_mut<SpringRenderer>();
            systems::draw_springs_gpu(ctx);
        }).disable(0);

    // upload particle positions each frame before rendering
    ecs.system("graphics::UploadParticlePositions")
        .kind(graphics::phase_pre_render)
        .run([](flecs::iter& it) {
            auto& ctx = it.world().get_mut<ParticleRenderer>();
            systems::upload_particle_positions_to_gpu(it.world(), ctx);
        }).disable(0);

    ecs.system("graphics::DrawParticlesGPU")
        .kind(graphics::phase_on_render)
        .run([](flecs::iter& it) {
            auto& ctx = it.world().get_mut<ParticleRenderer>();
            systems::draw_particles_gpu(ctx);
        }).disable(0);

    ecs.system("graphics::DrawTimingInfo")
        .kind(graphics::phase_post_render)
        .run(systems::draw_timing_info);

    ecs.system("graphics::DrawSolveHistory")
        .kind(graphics::phase_post_render)
        .run(systems::draw_solve_history)
        .disable();

    // interaction (still graphics-facing for now)
    ecs.system("graphics::PickParticles")
        .kind(flecs::OnLoad)
        .run([&](flecs::iter& it) {
            auto& interaction = it.world().get_mut<InteractionState>();
            auto& renderer = it.world().get<ParticleRenderer>();

            // ray pick -> nearest particle
            Ray ray = GetMouseRay(GetMousePosition(), graphics::get_raylib_camera_const());
            float pick_radius = renderer.base_radius * interaction.pick_radius_scale;

            float closest = std::numeric_limits<float>::max();
            flecs::entity hovered = flecs::entity::null();

            queries::particle_pick_query.each([&](flecs::entity e, const Position& pos, ParticleState&) {
                RayCollision hit = GetRayCollisionSphere(ray, {pos[0], pos[1], pos[2]}, pick_radius);
                if (hit.hit && hit.distance < closest) {
                    closest = hit.distance;
                    hovered = e;
                }
            });

            // TODO: do gpu picking maybe?
            // i want no linear scan every frame looping all the particles..

            interaction.hovered = hovered;

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                if (hovered.is_alive()) {
                    interaction.selected = (interaction.selected == hovered)
                        ? flecs::entity::null()
                        : hovered;
                } else {
                    interaction.selected = flecs::entity::null();
                }
            }

            // if hit, lock camera drag interaction
            bool capture = IsMouseButtonDown(MOUSE_LEFT_BUTTON) && hovered.is_alive();
            it.world().query_builder<graphics::Camera>()
                .with<graphics::ActiveCamera>()
                .each([&](graphics::Camera& cam) {
                    cam.controls_enabled = !capture;
                });

            // update hover/selection flag bits (feels overengineered)
            constexpr uint32_t kInteractionMask =
                ParticleState::Hovered | ParticleState::Selected;

            queries::particle_pick_query.each([&](flecs::entity e, const Position&, ParticleState& state) {
                uint32_t flags = (uint32_t)(state.flags & ~kInteractionMask);
                if (e == hovered) flags |= ParticleState::Hovered;
                if (interaction.selected == e) flags |= ParticleState::Selected;
                state.flags = flags;
            });
        }).disable(0);
}

} // namespace systems
