#include <flecs.h>
#include <raylib.h>

#include "graphics.h"
#include <iostream>

#include "types.hpp"
#include "kernels.hpp"

float randf(int n) {
    return static_cast<float>(rand() % n);
}

float global_time = 0.0f;
float delta_time = 0.016f;

int main() {
    flecs::world ecs;

    graphics::init(ecs);
    graphics::init_window(800, 800, "Physics Simulator");

    // graphics2: use graphics::PostRender instead of graphics::PostRender
    ecs.system("DrawTimingInfo").kind(graphics::PostRender).run([](flecs::iter& it) {
        DrawText("If tilapia can do it, so can you!!!", 200, 200, 40, DARKGREEN);
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
