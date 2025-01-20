#include "flecs.h"

#include "graphics.h"
#include <iostream>

float randf(int n) {
    return static_cast<float>(rand() % n);
}

float global_time = 0.0f;
float delta_time = 0.016f;

using namespace rl;

int main() {
    flecs::world ecs;

    graphics::init(ecs);
    graphics::init_window(800, 800, "Physics Simulator");

    Vector2 startPoint = {30, 30};
    Vector2 endPoint = {(float)800 - 30, (float)800 - 30};
    bool moveStartPoint = false; // start.is_mouse_down()
    bool moveEndPoint = false; // end.is_mouse_down()

    ecs.system("mouse update").kind(flecs::OnUpdate).run([&](flecs::iter& it) {});

    ecs.system("you can do it").kind(graphics::PostRender).run([&](flecs::iter& it) {
        Vector2 mouse = GetMousePosition();

        if (CheckCollisionPointCircle(mouse, startPoint, 10.0f) &&
            IsMouseButtonDown(MOUSE_BUTTON_LEFT))
            moveStartPoint = true;
        else if (CheckCollisionPointCircle(mouse, endPoint, 10.0f) &&
                 IsMouseButtonDown(MOUSE_BUTTON_LEFT))
            moveEndPoint = true;

        if (moveStartPoint) {
            startPoint = mouse;
            if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
                moveStartPoint = false;
        }

        if (moveEndPoint) {
            endPoint = mouse;
            if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
                moveEndPoint = false;
        }

        ClearBackground(RAYWHITE);

        DrawText("MOVE START-END POINTS WITH MOUSE", 15, 20, 20, GRAY);

        // Draw line Cubic Bezier, in-out interpolation (easing), no control points
        DrawLineBezier(startPoint, endPoint, 4.0f, BLUE);

        // Draw start-end spline circles with some details
        DrawCircleV(startPoint,
                    CheckCollisionPointCircle(mouse, startPoint, 10.0f) ? 14.0f : 8.0f,
                    moveStartPoint ? RED : BLUE);
        
        DrawCircleV(endPoint,
                    CheckCollisionPointCircle(mouse, endPoint, 10.0f) ? 14.0f : 8.0f,
                    moveEndPoint ? RED : BLUE);
    });

    graphics::run_main_loop([]() { global_time += delta_time; });

    std::cout << "Simulation ended." << std::endl;
    return 0;
}