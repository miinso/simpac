#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>

#include <flecs.h>
#include <raylib.h>

#include "graphics.h"

#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#else
#define EMSCRIPTEN_KEEPALIVE
#endif

namespace {
    std::vector<void *> g_blocks;
    size_t g_bytes = 0;
} // namespace

extern "C" {
EMSCRIPTEN_KEEPALIVE void leak_hold(uint32_t bytes) {
    if (bytes == 0) {
        return;
    }
    void *ptr = std::malloc(bytes);
    if (!ptr) {
        return;
    }
    std::memset(ptr, 0xA5, bytes);
    g_blocks.push_back(ptr);
    g_bytes += bytes;
}

EMSCRIPTEN_KEEPALIVE void leak_clear() {
    for (void *ptr: g_blocks) {
        std::free(ptr);
    }
    g_blocks.clear();
    g_bytes = 0;
}

EMSCRIPTEN_KEEPALIVE uint32_t leak_held_bytes() { return static_cast<uint32_t>(g_bytes); }

EMSCRIPTEN_KEEPALIVE uint32_t leak_block_count() { return static_cast<uint32_t>(g_blocks.size()); }
}

void UpdateDrawFrame(void);

int main() {
    flecs::world world;

    graphics::init(world);
    graphics::init_window(800, 600, "Leak Engine");

    world.system("DrawOverlay").kind(graphics::PostRender).run([](flecs::iter &it) {
        static int counter = 0;
        Vector2 m = GetMousePosition();
        DrawText(TextFormat("Hello from engine: %i", counter++), 20, 40, 20, DARKGREEN);
        DrawText(TextFormat("Mouse: %.0f, %.0f (Btn0: %d)", m.x, m.y, IsMouseButtonDown(0)), 20, 70,
                 20, DARKGREEN);
        // std::printf("counter %f\n", it.delta_system_time());
        // UpdateDrawFrame();
    });
    world.app().run();

    // "this works"
    // flecs::world world;
    // graphics::init(world);
    // graphics::init_window(800, 600, "Leak Engine");
    // world.system().run([](flecs::iter& it) {
    //     UpdateDrawFrame();
    // });
    // world.app().run();

    // "this works"
    // int screenWidth = 800;
    // int screenHeight = 450;
    // InitWindow(screenWidth, screenHeight, "raylib [others] example - web basic window");
    // emscripten_set_main_loop(UpdateDrawFrame, 0, 1);
    // CloseWindow();

    return 0;
}

void UpdateDrawFrame(void) {
    // Update
    //----------------------------------------------------------------------------------
    // TODO: Update your variables here
    //----------------------------------------------------------------------------------

    // Draw
    //----------------------------------------------------------------------------------
    BeginDrawing();

    ClearBackground(RAYWHITE);

    DrawText("Congrats! You created your first window!", 190, 200, 20, LIGHTGRAY);
    DrawFPS(20, 40);

    EndDrawing();
    //----------------------------------------------------------------------------------
}
