#include "graphics.h"
#include <flecs.h>
#include <cstdio>
#include "rlgl.h"

struct Counter {
    int value;
};

struct Spin {
    float angle;
};

int main() {
    std::printf("[explorer] start\n");

    flecs::world world;

    world.component<Counter>()
        .member<int>("value");
    world.component<Spin>()
        .member<float>("angle");

    world.entity("Counter").set<Counter>({0});
    world.entity("Spinner").set<Spin>({0.0f});

    world.system<Counter>("TickCounter")
        .kind(flecs::OnUpdate)
        .each([](Counter& counter) {
            counter.value += 1;
        }).disable();

    world.system<Spin>("AdvanceSpin")
        .kind(flecs::OnUpdate)
        .each([](flecs::iter& it, size_t, Spin& spin) {
            spin.angle += 90.0f * it.delta_time();
            if (spin.angle > 360.0f) {
                spin.angle -= 360.0f;
            }
        });

    graphics::init(world);
    graphics::init_window(800, 600, "Flecs Explorer Demo");

    graphics::Camera cam{};
    cam.position[0] = 6.0f;
    cam.position[1] = 5.0f;
    cam.position[2] = 6.0f;
    cam.target[1] = 1.0f;
    graphics::create_camera(world, "MainCamera", cam, true);

    world.system<const Spin>("DrawScene")
        .kind(graphics::phase_on_render)
        .each([](const Spin& spin) {
            rlPushMatrix();
                rlRotatef(spin.angle, 0, 1, 0);
                DrawCube({0.0f, 1.0f, 0.0f}, 1.0f, 1.0f, 1.0f, ORANGE);
                DrawCubeWires({0.0f, 1.0f, 0.0f}, 1.0f, 1.0f, 1.0f, DARKGRAY);
                DrawSphere({2.0f, 1.0f, 0.0f}, 0.5f, SKYBLUE);
            rlPopMatrix();
        });

    world.app()
        .enable_rest()
        .enable_stats()
        .run();

    std::printf("[explorer] end\n");
    return 0;
}
