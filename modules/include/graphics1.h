// graphics.h
#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "flecs.h"
#include <functional>

// people say wrapping header this way is a big no-no
// namespace rl {
// #include "raylib.h"
// // #include "raymath.h"
// // #include "rlgl.h"
// } // namespace rl

#include "raylib-cpp.hpp"

namespace rl = raylib;


namespace graphics {
    // global state
    extern flecs::world* ecs;
    extern std::function<void()> update_func;
    extern rl::Window* window;

    extern flecs::entity PreRender;
    extern flecs::entity OnRender;
    extern flecs::entity PostRender;
    extern flecs::entity OnPresent;

    extern rl::Camera3D camera1;
    extern rl::Camera3D camera2;
    extern int active_camera;

    void init(flecs::world& ecs);
    void init_window(int width, int height, const char* title);
    void run_main_loop(std::function<void()> update = nullptr);
    bool window_should_close();
    void close_window();
} // namespace graphics

#endif // GRAPHICS_H