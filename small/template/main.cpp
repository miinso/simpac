#include <flecs.h>
#include <raylib.h>

#include "graphics.h"

int main() {
    flecs::world world;

    graphics::init(world);
    graphics::init_window(800, 600, "Template");

    world.system("DrawOverlay")
        .kind(graphics::phase_post_render)
        .run([](flecs::iter&) {
            DrawText("Hello from template", 20, 40, 20, DARKGREEN);
        });

    world.app().run();
    // For Explorer: world.app().enable_rest().enable_stats().run();
    // or manually: graphics::run_loop();

    return 0;
}
