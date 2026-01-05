#include "flecs.h"

#include "graphics.h"
#include "raylib.h"
#include <cstdio>

float randf(int n) {
    return static_cast<float>(rand() % n);
}

float global_time = 0.0f;
float delta_time = 0.016f;

int main() {
    flecs::world ecs;
    
    graphics::init(ecs);
    graphics::init_window(640, 480, "Physics Simulator");

    ecs.system("DrawTimingInfo").kind(graphics::PostRender).run([](flecs::iter& it) {
        
        DrawText("If tilapia can do it, so can you", 200, 200, 40, DARKGREEN);
        DrawText(TextFormat("Global Time: %.2f", global_time), 20, 40, 20, BLUE);
    });

    graphics::run_main_loop([]() { global_time += delta_time; });

    // std::cout << "Simulation ended." << std::endl;
    printf("Simulation ended");
    return 0;
}

