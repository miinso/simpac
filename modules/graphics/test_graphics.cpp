// Simple test for the unified graphics module
#include "graphics.h"

int main() {
    flecs::world ecs;

    // Initialize graphics
    graphics::init(ecs);
    graphics::init_window(800, 600, "Graphics Test");

    // Optional: enable lighting
    // graphics::enable_lighting(
    //     TextFormat("resources/shaders/glsl%i/lighting.vs", graphics::GLSL_VERSION),
    //     TextFormat("resources/shaders/glsl%i/lighting.fs", graphics::GLSL_VERSION)
    // );
    // graphics::set_ambient_light(0.2f, 0.2f, 0.2f);

    // Register a custom system to draw something
    ecs.system("draw_test")
        .kind(graphics::phase_on_render)
        .run([](flecs::iter& it) {
            // Draw some test geometry
            DrawCube({0, 0, 0}, 1.0f, 1.0f, 1.0f, RED);
            DrawCubeWires({0, 0, 0}, 1.0f, 1.0f, 1.0f, MAROON);

            DrawSphere({2, 0, 0}, 0.5f, BLUE);
            DrawSphereWires({2, 0, 0}, 0.5f, 8, 8, DARKBLUE);

            DrawCylinder({-2, 0, 0}, 0.0f, 0.5f, 1.0f, 8, GREEN);
        });

    // Run the main loop
    graphics::run_main_loop();

    return 0;
}
