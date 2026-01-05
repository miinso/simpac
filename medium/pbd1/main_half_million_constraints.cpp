#include <flecs.h>
#include <raylib.h>

#include "components.hpp"
#include "graphics.h"
#include "pbd1.hpp"
#include <iostream>
#include <vector>

float randf(int n) {
    return static_cast<float>(rand() % n);
}

float delta_time = 0.01f;

Vector3f gravity(0, -9.81f, 0);
// Vector3f gravity(0, 0, 0);

int main() {
    flecs::world ecs;
    // ecs.set_threads(16); // it just works??

    graphics::init(ecs);
    graphics::init_window(800, 800, "muller2007position");

    ecs.set<Scene>({delta_time, 1, 1, gravity});

    std::vector<flecs::entity> particles;
    int num_particles = 1000;
    float position_range = 10.0f;

    for (int i = 0; i < num_particles; ++i) {
        Vector3f position((randf(200) / 100.0f - 1.0f) * 1.0f,
                          (randf(200) / 100.0f - 1.0f) * 1.0f + 2,
                          (randf(200) / 100.0f - 1.0f) * 1.0f);

        Vector3f velocity((randf(200) / 100.0f - 1.0f) * 1.0f,
                          (randf(200) / 100.0f - 1.0f) * 1.0f,
                          (randf(200) / 100.0f - 1.0f) * 1.0f);
        auto p = add_particle(ecs, position, 1.0f);
        p.get_mut<Velocity>().value = velocity * 10;
        particles.push_back(p);
    }

    float stiffness = 1e+3;
    for (size_t i = 0; i < particles.size(); ++i) {
        for (size_t j = i + 1; j < particles.size(); ++j) {
            add_distance_constraint(ecs, particles[i], particles[j], stiffness);
        }
    }

    ////////// system registration
    ecs.system("progress").kind(flecs::OnUpdate).run([](flecs::iter& it) {
        auto& scene = it.world().get_mut<Scene>();
        scene.wall_time += it.delta_time();
        scene.sim_time += scene.dt;
        scene.frame_count++;
    });

    ecs.system<Acceleration>("clear acceleration")
        .with<Particle>()
        .multi_threaded()
        .each(pbd_particle_clear_acceleration);

    ecs.system<Position, OldPosition, Mass, Velocity, Acceleration>("integrate")
        .with<Particle>()
        .multi_threaded()
        .each(pbd_particle_integrate);

    ecs.system("clear constraint lambda")
        .with<Constraint>()
        .multi_threaded()
        .each(pbd_particle_clear_constraint_lambda);

    ecs.system("constraint solve")
        .with<Constraint>()
        .multi_threaded()
        .run(pbd_particle_solve_constraint);

    ecs.system<Position, const OldPosition>("ground collision")
        .with<Particle>()
        .each(pbd_particle_ground_collision);

    ecs.system<const Position, const OldPosition, Velocity>("update velocity")
        .with<Particle>()
        .multi_threaded()
        .each(pbd_particle_update_velocity);

    //////// drawings below
    // graphics2: use graphics::phase_on_render instead of graphics::OnRender
    ecs.system<Position>("draw_particle")
        .with<Particle>()
        .kind(graphics::phase_on_render)
        .each([](Position& x) {
            // DrawSphere({x.value.x(), x.value.y(), x.value.z()}, 0.05, BLUE);
            DrawPoint3D({x.value.x(), x.value.y(), x.value.z()}, BLUE);
        });

    auto q_particle = ecs.query_builder().with<Particle>().cached().build();
    auto q_constraint = ecs.query_builder().with<Constraint>().cached().build();

    // graphics2: use graphics::phase_post_render instead of graphics::PostRender
    ecs.system("DrawTimingInfo").kind(graphics::phase_post_render).run([&](flecs::iter& it) {
        auto constraint_count = q_constraint.count();
        auto particle_count = q_particle.count();

        const auto& scene = it.world().get<Scene>();

        Font font = graphics::get_font();
        char buffer[256];
        snprintf(buffer, sizeof(buffer),
            "Wall time: %.2fs  |  Sim time: %.2fs\n"
            "Frame: %d  |  Speed: %.2fx\n"
            "Particles: %d  |  Constraints: %d",
            scene.wall_time, scene.sim_time,
            scene.frame_count, scene.sim_time / (scene.wall_time + 1e-9f),
            particle_count, constraint_count);
        DrawTextEx(font, buffer, {21, 101}, 12, 0, WHITE);
        DrawTextEx(font, buffer, {20, 100}, 12, 0, DARKGREEN);
    });

    graphics::run_main_loop();

    std::cout << "Simulation ended." << std::endl;
    return 0;
}