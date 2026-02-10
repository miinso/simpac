#include <flecs.h>
#include <raylib.h>

#include "components.hpp"
#include "graphics.h"
#include "pbd1.hpp"
#include <iostream>

float randf(int n) {
    return static_cast<float>(rand() % n);
}

float global_time = 0.0f;
float delta_time = 0.01f;

Vector3f gravity(0, -9.81f, 0);
// Vector3f gravity(0, 0, 0);

int main() {
    flecs::world ecs;

    graphics::init(ecs);
    graphics::init_window(800, 800, "muller2007position");

    ecs.set<Scene>({delta_time, 1, 1, gravity});

    auto p1 = add_particle(ecs, Eigen::Vector3f(0, 2, 0), 1.0f);
    auto p2 = add_particle(ecs, Eigen::Vector3f(1, 1, 0), 1.0f);
    auto p3 = add_particle(ecs, Eigen::Vector3f(0, 1, 1), 1.0f);
    auto p4 = add_particle(ecs, Eigen::Vector3f(1, 1, 1), 1.0f);
    // Flecs 4.1.1+: get_mut<T>() returns reference, not pointer
    p2.get_mut<Velocity>().value = {1, 3, 1};

    add_distance_constraint(ecs, p1, p2, 1e+7);
    add_distance_constraint(ecs, p2, p3, 1e+7);
    add_distance_constraint(ecs, p3, p4, 1e+7);
    add_distance_constraint(ecs, p1, p4, 1e+7);
    add_distance_constraint(ecs, p2, p4, 1e+7);

    ////////// system registration
    ecs.system("progress").kind(flecs::OnUpdate).run([](flecs::iter& it) {
        auto& scene = it.world().get_mut<Scene>();
        scene.wall_time += it.delta_time();
        scene.sim_time += scene.dt;
        scene.frame_count++;
    });

    ecs.system<Acceleration>("clear acceleration")
        .with<Particle>()
        .each(pbd_particle_clear_acceleration);

    ecs.system<Position, OldPosition, Mass, Velocity, Acceleration>("integrate")
        .with<Particle>()
        .each(pbd_particle_integrate);

    ecs.system("clear constraint lambda")
        .with<Constraint>()
        .each(pbd_particle_clear_constraint_lambda);

    ecs.system("constraint solve").with<Constraint>().run(pbd_particle_solve_constraint);

    ecs.system<Position, const OldPosition>("ground collision")
        .with<Particle>()
        .each(pbd_particle_ground_collision);

    ecs.system<const Position, const OldPosition, Velocity>("update velocity")
        .with<Particle>()
        .each(pbd_particle_update_velocity);

    //////// drawings below
    // graphics2: use graphics::OnRender instead of graphics::OnRender
    ecs.system<Position>("draw_particle")
        .with<Particle>()
        .kind(graphics::OnRender)
        .each([](Position& x) {
            DrawSphere({x.value.x(), x.value.y(), x.value.z()}, 0.05, BLUE);
        });

    ecs.system<DistanceConstraint>("draw_distance_constraint")
        .kind(graphics::OnRender)
        .each([](DistanceConstraint& c) {
            auto e1 = c.e1;
            auto e2 = c.e2;

            // Flecs 4.1.1+: get_mut<T>() returns reference, not pointer
            auto x1 = e1.get_mut<Position>().value;
            auto x2 = e2.get_mut<Position>().value;

            // DrawLine3D({x1.x(), x1.y(), x1.z()}, {x2.x(), x2.y(), x2.z()}, GREEN);
            DrawCylinderEx(
                {x1.x(), x1.y(), x1.z()}, {x2.x(), x2.y(), x2.z()}, 0.02, 0.02, 4, GREEN);
        });

    // graphics2: use graphics::PostRender instead of graphics::PostRender
    // graphics2: use graphics::get_raylib_camera_const() instead of graphics::camera1
    ecs.system<DistanceConstraint>("draw_constraint_lambda")
        .kind(graphics::PostRender)
        .each([](DistanceConstraint& c) {
            auto e1 = c.e1;
            auto e2 = c.e2;

            // Flecs 4.1.1+: get_mut<T>() returns reference, not pointer
            auto x1 = e1.get_mut<Position>().value;
            auto x2 = e2.get_mut<Position>().value;

            auto x = (x1 + x2) / 2;

            Vector3 textPos = {x.x(), x.y(), x.z()};
            Vector2 screenPos = GetWorldToScreen(textPos, graphics::get_raylib_camera_const());

            char id_buffer[32];

            const char* label = (sprintf(id_buffer, "%0.6f", c.lambda), id_buffer);

            DrawText(label, screenPos.x, screenPos.y, 20, BLUE);
        });

    ecs.system("DrawTimingInfo").kind(graphics::PostRender).run([](flecs::iter& it) {
        Font font = graphics::get_font();

        const auto& scene = it.world().get<Scene>();

        char buffer[256];
        snprintf(buffer, sizeof(buffer),
            "Wall time: %.2fs  |  Sim time: %.2fs\n"
            "Frame: %d  |  Speed: %.2fx",
            scene.wall_time, scene.sim_time,
            scene.frame_count, scene.sim_time / (scene.wall_time + 1e-9f));
        DrawTextEx(font, buffer, {21, 101}, 12, 0, WHITE);
        DrawTextEx(font, buffer, {20, 100}, 12, 0, DARKGREEN);
    });

    ecs.system("TickTime")
        .kind(flecs::PreUpdate)
        .run([](flecs::iter&) {
            global_time += delta_time;
        });

    graphics::run_loop();

    std::cout << "Simulation ended." << std::endl;
    return 0;
}
