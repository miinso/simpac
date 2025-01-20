// 2d renderer

// graphics.cpp
#include "graphics.h"
#include <iostream>

#if defined(__EMSCRIPTEN__)
#    include <emscripten/emscripten.h>
#endif

// TODO: use 300es for both
#if defined(PLATFORM_DESKTOP)
#    define GLSL_VERSION 330
#else // PLATFORM_ANDROID, PLATFORM_WEB
#    define GLSL_VERSION 100
#endif

namespace graphics {
    // global states
    flecs::world* ecs = nullptr;
    std::function<void()> update_func;

    flecs::entity PreRender;
    flecs::entity OnRender;
    flecs::entity PostRender;
    flecs::entity OnPresent;

    rl::Camera3D camera{};

    static void init_default_camera() {
        camera.position = {5.0f, 5.0f, 5.0f};
        camera.target = {0.0f, 0.0f, 0.0f};
        camera.up = {0.0f, 1.0f, 0.0f};
        camera.fovy = 60.0f;
        camera.projection = rl::CAMERA_PERSPECTIVE;
    }

    static void setup_render_pipeline() {
        ecs->system("graphics::begin").kind(PreRender).run([](flecs::iter& it) {
            rl::BeginDrawing();
            rl::ClearBackground(rl::RAYWHITE);

            float cameraPos[3] = {camera.position.x, camera.position.y, camera.position.z};
        });

        ecs->system("graphics::render3d").kind(OnRender).run([](flecs::iter& it) {
            rl::BeginMode3D(camera);
        });

        ecs->system("graphics::render2d").kind(PostRender).run([](flecs::iter& it) {
            rl::DrawGrid(12, 10.0f / 12);
            rl::EndMode3D();
            rl::DrawFPS(20, 20);
        });

        ecs->system("graphics::end").kind(OnPresent).run([](flecs::iter& it) { rl::EndDrawing(); });

        ecs->system("graphics::update_camera").kind(flecs::OnLoad).run([](flecs::iter& it) {
            rl::UpdateCameraPro(
                &camera,
                {rl::IsKeyDown(rl::KEY_W) * 0.1f - rl::IsKeyDown(rl::KEY_S) * 0.1f,
                 rl::IsKeyDown(rl::KEY_D) * 0.1f - rl::IsKeyDown(rl::KEY_A) * 0.1f,
                 rl::IsKeyDown(rl::KEY_E) * 0.1f - rl::IsKeyDown(rl::KEY_Q) * 0.1f},
                {rl::GetMouseDelta().x * rl::IsMouseButtonDown(rl::MOUSE_LEFT_BUTTON) * 0.2f +
                     (IsKeyDown(rl::KEY_RIGHT) * 1.5f - IsKeyDown(rl::KEY_LEFT) * 1.5f),
                 rl::GetMouseDelta().y * rl::IsMouseButtonDown(rl::MOUSE_LEFT_BUTTON) * 0.2f +
                     IsKeyDown(rl::KEY_DOWN) * 1.5f - IsKeyDown(rl::KEY_UP) * 1.5f,
                 0.0f},
                rl::GetMouseWheelMove() * -1.0f);
        });
    }

#if defined(__EMSCRIPTEN__)
    static void main_loop_rAF_callback() {
        if (ecs) {
            ecs->progress();
        }

        if (update_func) {
            update_func();
        }
    }
#endif

    void init(flecs::world& world) {
        ecs = &world;

        // provide custom phases to flecs
        PreRender = ecs->entity("PreRender").add(flecs::Phase).depends_on(flecs::PostUpdate);
        OnRender = ecs->entity("OnRender").add(flecs::Phase).depends_on(PreRender);
        PostRender = ecs->entity("PostRender").add(flecs::Phase).depends_on(OnRender);
        OnPresent = ecs->entity("OnPresent").add(flecs::Phase).depends_on(PostRender);

        setup_render_pipeline();
    }

    void init_window(int width, int height, const char* title) {
        rl::InitWindow(width, height, title);
        init_default_camera();
    }

    bool window_should_close() {
        return rl::WindowShouldClose();
    }

    void close_window() {
        rl::CloseWindow();
    }

    void run_main_loop(std::function<void()> update) {
        update_func = update;

#if defined(__EMSCRIPTEN__)
        std::cout << "graphics::invoking emscripten rAF loop" << std::endl;
        emscripten_set_main_loop(main_loop_rAF_callback, 0, 1);
#else
        rl::SetTargetFPS(60);
        std::cout << "graphics::invoking native while loop" << std::endl;
        while (!window_should_close()) {
            if (ecs) {
                ecs->progress(0.01);
            }

            if (update_func) {
                update_func();
            }
        }
        std::cout << "graphics::destroying native while loop" << std::endl;
        close_window();
#endif
    }
} // namespace graphics
