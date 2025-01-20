// graphics.h
#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "flecs.h"
#include <functional>

#include "raylib.h"
#include "raymath.h"
namespace rl {
} // namespace rl

namespace graphics {
    // global state
    extern flecs::world* ecs;
    extern std::function<void()> update_func;

    extern flecs::entity PreRender;
    extern flecs::entity OnRender;
    extern flecs::entity PostRender;
    extern flecs::entity OnPresent;

    extern Camera3D camera;

    void init(flecs::world& ecs);
    void init_window(int width, int height, const char* title);
    void run_main_loop(std::function<void()> update = nullptr);
    bool window_should_close();
    void close_window();
} // namespace graphics

#endif // GRAPHICS_H