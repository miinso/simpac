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

    // graphics2: use graphics::phase_post_render instead of graphics::PostRender
    ecs.system("you can do it").kind(graphics::phase_post_render).run([](flecs::iter& it) {
        DrawText("You Can DO iT!!!", 200, 200, 40, DARKGREEN);
    });

    graphics::run_main_loop([]() { global_time += delta_time; });

    std::cout << "Simulation ended." << std::endl;
    return 0;
}