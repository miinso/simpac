#pragma once

#include "components.h"
#include "graphics.h"
#include "flecs.h"
#include <raylib.h>
#include <rlgl.h>
#include <raymath.h>

#ifdef __EMSCRIPTEN__
    #include <GLES3/gl3.h> // for browser
#else
    #include <glad/gles2.h>
#endif

#include <cmath>

namespace systems {

constexpr double kEpsilon = 1e-6;

// =========================================================================
// Simulation kernels
// =========================================================================

inline void update_velocity(Velocity& v, const ParticleIndex& idx, const Solver& solver) {
    v.value = solver.x.segment<3>(idx.value * 3);
}

inline void update_position(Position& x, const Velocity& v, Real dt) {
    x.value += dt * v.value;
}

// inline void clear_acceleration(Acceleration& a, const Vector3r& gravity) {
//     a.value = gravity;
// }

inline void collect_external_force(const Mass& mass, const Velocity& vel, 
                                   const ParticleIndex& idx, const Vector3r& gravity,
                                   Real dt, Solver& solver) {
    // RHS contribution: M*v_n + h*f_ext
    // where f_ext = m*g (gravity)
    Vector3r f_gravity = mass.value * gravity;
    solver.b.segment<3>(idx.value * 3) += mass.value * vel.value + dt * f_gravity;
}

inline void collect_spring_gradient(Spring& spring, Real dt, Solver& solver) {
    auto a = spring.particle_a;
    auto b = spring.particle_b;

    bool a_pinned = a.has<IsPinned>();
    bool b_pinned = b.has<IsPinned>();

    auto x_a = a.get<Position>().value;
    auto x_b = b.get<Position>().value;
    auto v_a = a.get<Velocity>().value;
    auto v_b = b.get<Velocity>().value;
    auto idx_a = a.get<ParticleIndex>().value;
    auto idx_b = b.get<ParticleIndex>().value;

    auto diff = x_a - x_b; // we follow stuyck2018cloth. x_ij = x_i - x_j
    auto l = diff.norm();
    if (l < kEpsilon) return;
    auto dir = diff / l;
    
    auto strain = (l - spring.rest_length) / spring.rest_length; // make it unitless

    auto diff_v = v_a - v_b;
    auto relative_velocity = (diff_v / spring.rest_length).dot(dir);

    Vector3r grad_a = (spring.k_s * strain + spring.k_d * relative_velocity) * dir; // grad_a = -force_a
    Vector3r grad_b = -grad_a;

    // RHS contribution: -h*∇E (elastic forces)
    // Only add contributions for non-pinned particles
    if (!a_pinned) solver.b.segment<3>(idx_a * 3) += -dt * grad_a;
    if (!b_pinned) solver.b.segment<3>(idx_b * 3) += -dt * grad_b;
}

inline void collect_mass(const Mass& m, const ParticleIndex& idx, Solver& solver) {
    for (int i = 0; i < 3; ++i) {
        solver.triplets.push_back({idx.value*3+i, idx.value*3+i, m.value});
    }
}

inline void collect_spring_hessian(Spring& spring, Real dt, Solver& solver) {
    // maybe i should merge grad/hess evals into one system
    auto a = spring.particle_a;
    auto b = spring.particle_b;

    bool a_pinned = a.has<IsPinned>();
    bool b_pinned = b.has<IsPinned>();

    auto x_a = a.get<Position>().value;
    auto x_b = b.get<Position>().value;
    auto idx_a = a.get<ParticleIndex>().value;
    auto idx_b = b.get<ParticleIndex>().value;

    auto diff = x_a - x_b;
    auto l = diff.norm();
    if (l < kEpsilon) return;

    auto dir = diff / l;

    Real k = spring.k_s;
    Real L0 = spring.rest_length;
    Real ratio = L0 / l;

    Matrix3r I = Matrix3r::Identity();
    Matrix3r ddT = dir * dir.transpose();
    Matrix3r H_block = k * ((1.0 - ratio) * I + ratio * ddT);

    Real h2 = dt * dt;

    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            Real val = h2 * H_block(r, c);
            // diagonal blocks
            if (!a_pinned) solver.triplets.push_back({idx_a*3+r, idx_a*3+c, val});
            if (!b_pinned) solver.triplets.push_back({idx_b*3+r, idx_b*3+c, val});
            // off-diagonal blocks (only if both particles are not pinned)
            if (!a_pinned && !b_pinned) {
                solver.triplets.push_back({idx_a*3+r, idx_b*3+c, -val});
                solver.triplets.push_back({idx_b*3+r, idx_a*3+c, -val});
            }
        }
    }
}


inline void solve(Solver& solver) {
    // build sparse mat
    solver.A.setFromTriplets(solver.triplets.begin(), solver.triplets.end());

    solver.cg.setMaxIterations(solver.cg_max_iter);
    solver.cg.setTolerance(solver.cg_tolerance);
    
    if (solver.cg.info() == Eigen::InvalidInput) {
        solver.cg.compute(solver.A);
    } else {
        solver.cg.factorize(solver.A);
    }

    solver.x = solver.cg.solveWithGuess(solver.b, solver.x_prev); // warm start, should check same size
    // solver.x = solver.cg.solve(solver.b); // cold start
    solver.x_prev = solver.x;

    // store stats for debug display
    solver.cg_iterations = (int)solver.cg.iterations();
    solver.cg_error = solver.cg.error();

    static int frame = 0;
    if (++frame % 50 == 0 || solver.cg.info() != Eigen::Success) {
        char buf[128];
        snprintf(buf, sizeof(buf), "[%d] CG: %d iter, err=%e%s",
                 frame, solver.cg_iterations, solver.cg_error,
                 solver.cg.info() != Eigen::Success ? " FAILED" : "");

        printf("%s\n", buf);

        solver.cg_history.push_back(buf);
        if (solver.cg_history.size() > (size_t)solver.cg_history_max_lines) {
            solver.cg_history.pop_front();
        }
    }
}


inline void ground_collision(Position& x, const OldPosition& x_old, Real dt, Real friction = 0.8) {
    if (x.value.y() < 0) {
        x.value.y() = 0;
        auto displacement = x.value - x_old.value;
        auto friction_factor = std::min(Real(1), dt * friction);
        x.value.x() = x_old.value.x() + displacement.x() * (1 - friction_factor);
        x.value.z() = x_old.value.z() + displacement.z() * (1 - friction_factor);
    }
}


// =========================================================================
// Drawing
// =========================================================================

inline Vector3 toRay3(const Vector3r& v) {
    return {(float)v.x(), (float)v.y(), (float)v.z()};
}

inline void draw_spring(Spring& spring) {
    auto& x1 = spring.particle_a.get<Position>().value;
    auto& x2 = spring.particle_b.get<Position>().value;
    
    auto diff = x1 - x2;
    auto current_length = diff.norm();
    auto strain = (current_length - spring.rest_length) / spring.rest_length;
    
    // Color based on strain: compression(blue) -> rest(green) -> tension(red)
    Color color;
    if (strain > 0) {
        float t = std::min(1.0f, (float)strain * 10.0f);
        color = ColorLerp(GREEN, RED, t);
    } else {
        float t = std::min(1.0f, (float)(-strain) * 10.0f);
        color = ColorLerp(GREEN, BLUE, t);
    }
    
    DrawLine3D(toRay3(x1), toRay3(x2), color);
}

inline void draw_particle(const Position& x, const Mass& m) {
    DrawPoint3D(toRay3(x.value), BLUE);
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
        scene.num_particles(), scene.num_springs());
    DrawTextEx(font, buf, {21, 41}, 12, 0, WHITE);
    DrawTextEx(font, buf, {20, 40}, 12, 0, DARKGRAY);

    // Draw CG history (right-aligned)
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
// GPU Spring Rendering
// =========================================================================

inline void upload_spring_positions_to_gpu(flecs::world& ecs, SpringRenderer& gpu) {
    if (gpu.shader_id == 0) return;

    auto& scene = ecs.get<Scene>();
    int num_particles = scene.num_particles();
    int num_springs = scene.num_springs();

    // Lazy buffer allocation/reallocation on topology change
    if (gpu.allocated_springs != num_springs) {
        gpu.allocated_springs = num_springs;

        // Collect spring connectivity
        gpu.spring_particle_indices.clear();
        gpu.spring_particle_indices.reserve(num_springs * 2);
        gpu.rest_lengths.clear();
        gpu.rest_lengths.reserve(num_springs);

        ecs.each<Spring>([&](flecs::entity e, Spring& s) {
            gpu.spring_particle_indices.push_back(s.particle_a.get<ParticleIndex>().value);
            gpu.spring_particle_indices.push_back(s.particle_b.get<ParticleIndex>().value);
            gpu.rest_lengths.push_back((float)s.rest_length);
        });

        // Resize staging buffer
        gpu.staging_buffer.resize(num_springs * 2 * 8);

        // Recreate GPU buffers
        if (gpu.vao) rlUnloadVertexArray(gpu.vao);
        if (gpu.vbo) rlUnloadVertexBuffer(gpu.vbo);

        gpu.vao = rlLoadVertexArray();
        rlEnableVertexArray(gpu.vao);
        gpu.vbo = rlLoadVertexBuffer(nullptr, gpu.staging_buffer.size() * sizeof(float), true);
        rlDisableVertexArray();
    }

    if (num_springs == 0) return;

    // Collect positions using cached query with run + iter pattern
    std::vector<Vector3r> positions(num_particles);
    gpu.position_query.run([&](flecs::iter& it) {
        while (it.next()) {
            auto pos = it.field<const Position>(0);
            auto idx = it.field<const ParticleIndex>(1);
            for (auto i : it) {
                positions[idx[i].value] = pos[i].value;
            }
        }
    });

    // Build vertex buffer (2 vertices per spring)
    int vert_idx = 0;
    for (int i = 0; i < num_springs; ++i) {
        int idx_a = gpu.spring_particle_indices[i * 2];
        int idx_b = gpu.spring_particle_indices[i * 2 + 1];
        Vector3r pa = positions[idx_a];
        Vector3r pb = positions[idx_b];
        float rest = gpu.rest_lengths[i];

        // Vertex 0 (start)
        gpu.staging_buffer[vert_idx++] = (float)pa.x();
        gpu.staging_buffer[vert_idx++] = (float)pa.y();
        gpu.staging_buffer[vert_idx++] = (float)pa.z();
        gpu.staging_buffer[vert_idx++] = (float)pb.x();
        gpu.staging_buffer[vert_idx++] = (float)pb.y();
        gpu.staging_buffer[vert_idx++] = (float)pb.z();
        gpu.staging_buffer[vert_idx++] = rest;
        gpu.staging_buffer[vert_idx++] = 0.0f;

        // Vertex 1 (end)
        gpu.staging_buffer[vert_idx++] = (float)pa.x();
        gpu.staging_buffer[vert_idx++] = (float)pa.y();
        gpu.staging_buffer[vert_idx++] = (float)pa.z();
        gpu.staging_buffer[vert_idx++] = (float)pb.x();
        gpu.staging_buffer[vert_idx++] = (float)pb.y();
        gpu.staging_buffer[vert_idx++] = (float)pb.z();
        gpu.staging_buffer[vert_idx++] = rest;
        gpu.staging_buffer[vert_idx++] = 1.0f;
    }

    rlUpdateVertexBuffer(gpu.vbo, gpu.staging_buffer.data(),
                         gpu.staging_buffer.size() * sizeof(float), 0);
}

inline void draw_springs_gpu(SpringRenderer& gpu) {
    if (gpu.shader_id == 0) return;

    // setup shader and uniforms
    Matrix viewproj = MatrixMultiply(rlGetMatrixModelview(), rlGetMatrixProjection());
    rlEnableShader(gpu.shader_id);
    rlSetUniformMatrix(gpu.u_viewproj_loc, viewproj);
    float strain_scale = 10.0f;
    rlSetUniform(gpu.u_strain_scale_loc, &strain_scale, SHADER_UNIFORM_FLOAT, 1);

    // bind VAO/VBO and setup attributes
    rlEnableVertexArray(gpu.vao);
    rlEnableVertexBuffer(gpu.vbo);

    int stride = 8 * sizeof(float);
    rlEnableVertexAttribute(0);
    rlSetVertexAttribute(0, 3, RL_FLOAT, false, stride, 0);                      // pos_a
    rlEnableVertexAttribute(1);
    rlSetVertexAttribute(1, 3, RL_FLOAT, false, stride, 3 * sizeof(float));      // pos_b
    rlEnableVertexAttribute(2);
    rlSetVertexAttribute(2, 1, RL_FLOAT, false, stride, 6 * sizeof(float));      // rest_len
    rlEnableVertexAttribute(3);
    rlSetVertexAttribute(3, 1, RL_FLOAT, false, stride, 7 * sizeof(float));      // endpoint

    // draw all springs in single call
    glDrawArrays(GL_LINES, 0, gpu.allocated_springs * 2);

    // cleanup
    rlDisableVertexAttribute(0);
    rlDisableVertexAttribute(1);
    rlDisableVertexAttribute(2);
    rlDisableVertexAttribute(3);
    rlDisableVertexBuffer();
    rlDisableVertexArray();
    rlDisableShader();
}

} // namespace systems
