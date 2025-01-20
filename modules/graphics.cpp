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

#include "lights.hpp"

namespace graphics {
    // global states
    flecs::world* ecs = nullptr;
    std::function<void()> update_func;

    flecs::entity PreRender;
    flecs::entity OnRender;
    flecs::entity PostRender;
    flecs::entity OnPresent;

    static rl::Shader lighting_shader;
    static Light main_light; // store our main light

    rl::Camera3D camera1{};
    rl::Camera3D camera2{};
    int active_camera = 1;

    static void init_default_camera() {
        camera1.position = {5.0f, 5.0f, 5.0f};
        camera1.target = {0.0f, 0.0f, 0.0f};
        camera1.up = {0.0f, 1.0f, 0.0f};
        camera1.fovy = 60.0f;
        camera1.projection = rl::CAMERA_PERSPECTIVE;

        camera2.position = {0.1f, 5.0f, 0.0f};
        camera2.target = {0.0f, 0.0f, 0.0f};
        camera2.up = {0.0f, 1.0f, 0.0f};
        camera2.fovy = 60.0f;
        camera2.projection = rl::CAMERA_PERSPECTIVE;

        // init lighting shader
        lighting_shader =
            rl::LoadShader(rl::TextFormat("resources/shaders/glsl%i/lighting.vs", GLSL_VERSION),
                           rl::TextFormat("resources/shaders/glsl%i/lighting.fs", GLSL_VERSION));

        // setup shader locations
        lighting_shader.locs[rl::SHADER_LOC_VECTOR_VIEW] =
            rl::GetShaderLocation(lighting_shader, "viewPos");

        // set ambient light level
        int ambient_loc = rl::GetShaderLocation(lighting_shader, "ambient");
        float ambient[4] = {0.1f, 0.5f, 0.1f, 1.0f};
        rl::SetShaderValue(lighting_shader, ambient_loc, ambient, rl::SHADER_UNIFORM_VEC4);

        // create main light
        main_light = create_light(
            LIGHT_POINT, // type
            {2.0f, 3.0f, 2.0f}, // position
            {0.0f, 0.0f, 0.0f}, // target
            rl::RED, // color
            lighting_shader, // shader
            0 // light index
        );

        // initial update of light values in shader
        update_light(lighting_shader, main_light);
    }

    static void setup_render_pipeline() {
        ecs->system("graphics::begin").kind(PreRender).run([](flecs::iter& it) {
            rl::BeginDrawing();
            rl::ClearBackground(rl::RAYWHITE);

            // Update the shader with the camera view vector (points towards { 0.0f, 0.0f, 0.0f })
            float cameraPos[3] = {camera1.position.x, camera1.position.y, camera1.position.z};
            rl::SetShaderValue(lighting_shader,
                               lighting_shader.locs[rl::SHADER_LOC_VECTOR_VIEW],
                               cameraPos,
                               rl::SHADER_UNIFORM_VEC3);

            update_light(lighting_shader, main_light);
        });

        ecs->system("graphics::render3d").kind(OnRender).run([](flecs::iter& it) {
            auto& camera = (active_camera == 1) ? camera1 : camera2;
            rl::BeginMode3D(camera);

            // rl::BeginShaderMode(lighting_shader);
        });

        ecs->system("graphics::render2d").kind(PostRender).run([](flecs::iter& it) {
            // rl::EndShaderMode();
            rl::DrawGrid(12, 10.0f / 12);
            rl::EndMode3D();
            rl::DrawFPS(20, 20);
        });

        ecs->system("graphics::end").kind(OnPresent).run([](flecs::iter& it) { rl::EndDrawing(); });

        ecs->system("graphics::update_camera").kind(flecs::OnLoad).run([](flecs::iter& it) {
            auto& camera = (active_camera == 1) ? camera1 : camera2;

            // if (rl::IsKeyPressed(rl::KEY_ONE))
            //     active_camera = 1;
            // if (rl::IsKeyPressed(rl::KEY_TWO))
            //     active_camera = 2;

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

        // do i want this?
        // ecs->module<graphics>();

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
