// polyfill test: verifies mouse, touch, gesture, keyboard, wheel
// all work correctly through the worker DOM polyfill.
// 3D orbit camera uses the graphics module's touch-aware controls.

#include <flecs.h>
#include <raylib.h>
#include <raymath.h>

#include "graphics.h"

#include <cstdio>

namespace {

// input mode: tracks whether input is coming from mouse or touch.
// raylib maps single touch to GetMousePosition(), so we can't rely on
// mouse APIs alone -- we need to know the actual input source.
enum class InputMode { IDLE, MOUSE, TOUCH };

struct InputState {
    InputMode mode = InputMode::IDLE;
    float mouse_x = 0, mouse_y = 0;
    float wheel_x = 0, wheel_y = 0;
    float wheel_accum = 0;
    bool mouse_left = false;
    bool mouse_right = false;
    int touch_count = 0;
    int gesture = GESTURE_NONE;
    int last_gesture = GESTURE_NONE;
    float drag_x = 0, drag_y = 0;
    float pinch_angle = 0;
    int last_key = 0;
    char last_key_name[32] = {};
    bool key_up = false, key_down = false, key_left = false, key_right = false;
    bool key_space = false;
};

const char* gesture_name(int g) {
    switch (g) {
        case GESTURE_NONE:       return "NONE";
        case GESTURE_TAP:        return "TAP";
        case GESTURE_DOUBLETAP:  return "DOUBLETAP";
        case GESTURE_HOLD:       return "HOLD";
        case GESTURE_DRAG:       return "DRAG";
        case GESTURE_SWIPE_RIGHT:return "SWIPE_RIGHT";
        case GESTURE_SWIPE_LEFT: return "SWIPE_LEFT";
        case GESTURE_SWIPE_UP:   return "SWIPE_UP";
        case GESTURE_SWIPE_DOWN: return "SWIPE_DOWN";
        case GESTURE_PINCH_IN:   return "PINCH_IN";
        case GESTURE_PINCH_OUT:  return "PINCH_OUT";
        default:                 return "???";
    }
}

const char* mode_name(InputMode m) {
    switch (m) {
        case InputMode::IDLE:  return "IDLE";
        case InputMode::MOUSE: return "MOUSE";
        case InputMode::TOUCH: return "TOUCH";
    }
    return "?";
}

} // namespace

int main() {
    std::printf("polyfill test\n");

    flecs::world ecs;
    graphics::init(ecs, {800, 600, "Polyfill Test"});

    ecs.set<InputState>({});

    // update input state + mode
    ecs.system("UpdateInput")
        .kind(flecs::PreUpdate)
        .run([](flecs::iter& it) {
            auto& s = it.world().get_mut<InputState>();

            s.touch_count = GetTouchPointCount();

            // input mode state machine:
            // TOUCH when fingers are down.
            // MOUSE when mouse button/movement detected without touch.
            // IDLE when transitioning from TOUCH (mouse state is stale from
            // raylib's internal touch-to-mouse mapping).
            if (s.touch_count > 0) {
                s.mode = InputMode::TOUCH;
            } else if (s.mode == InputMode::TOUCH) {
                // just ended touch -- go IDLE, not MOUSE.
                // GetMousePosition() is stale (set by raylib's touch callback).
                // wait for real mouse activity to transition to MOUSE.
                s.mode = InputMode::IDLE;
            } else {
                // IDLE or MOUSE: check for real mouse activity.
                // IsMouseButtonDown is only set by actual mouse events, never by
                // touch, so it's a reliable signal for real mouse presence.
                Vector2 md = GetMouseDelta();
                bool has_mouse = (md.x != 0.0f || md.y != 0.0f) ||
                                 IsMouseButtonDown(MOUSE_LEFT_BUTTON) ||
                                 IsMouseButtonDown(MOUSE_RIGHT_BUTTON);
                if (has_mouse) s.mode = InputMode::MOUSE;
            }

            Vector2 m = GetMousePosition();
            s.mouse_x = m.x; s.mouse_y = m.y;
            Vector2 w = GetMouseWheelMoveV();
            s.wheel_x = w.x; s.wheel_y = w.y; s.wheel_accum += w.y;
            s.mouse_left = IsMouseButtonDown(MOUSE_LEFT_BUTTON);
            s.mouse_right = IsMouseButtonDown(MOUSE_RIGHT_BUTTON);

            int g = GetGestureDetected();
            s.gesture = g;
            if (g != GESTURE_NONE) s.last_gesture = g;
            if (g == GESTURE_DRAG) {
                Vector2 dv = GetGestureDragVector();
                s.drag_x = dv.x; s.drag_y = dv.y;
            }
            if (g == GESTURE_PINCH_IN || g == GESTURE_PINCH_OUT)
                s.pinch_angle = GetGesturePinchAngle();
            int key = GetKeyPressed();
            if (key > 0) { s.last_key = key; std::snprintf(s.last_key_name, sizeof(s.last_key_name), "%d", key); }
            s.key_up = IsKeyDown(KEY_UP); s.key_down = IsKeyDown(KEY_DOWN);
            s.key_left = IsKeyDown(KEY_LEFT); s.key_right = IsKeyDown(KEY_RIGHT);
            s.key_space = IsKeyDown(KEY_SPACE);
        });

    // draw 3D scene
    ecs.system("Draw3D")
        .kind(graphics::OnRender)
        .run([](flecs::iter&) {
            DrawGrid(20, 1.0f);
            DrawCube({0, 0.5f, 0}, 1, 1, 1, RED);
            DrawCubeWires({0, 0.5f, 0}, 1, 1, 1, MAROON);
            DrawCube({2, 0.5f, 0}, 1, 1, 1, GREEN);
            DrawCubeWires({2, 0.5f, 0}, 1, 1, 1, DARKGREEN);
            DrawCube({0, 0.5f, 2}, 1, 1, 1, BLUE);
            DrawCubeWires({0, 0.5f, 2}, 1, 1, 1, DARKBLUE);
            DrawCube({-2, 1.0f, -2}, 1, 2, 1, YELLOW);
            DrawCubeWires({-2, 1.0f, -2}, 1, 2, 1, ORANGE);
            DrawSphere({3, 0.5f, -3}, 0.5f, PURPLE);
        });

    // draw HUD overlay
    ecs.system("DrawHUD")
        .kind(graphics::PostRender)
        .run([](flecs::iter& it) {
            const auto& s = it.world().get<InputState>();
            int y = 20;
            const int sz = 16, dy = 20;
            auto line = [&](const char* text, Color col = LIGHTGRAY) {
                DrawText(text, 20, y, sz, col); y += dy;
            };

            line(TextFormat("screen: %dx%d  render: %dx%d",
                GetScreenWidth(), GetScreenHeight(), GetRenderWidth(), GetRenderHeight()));
            line(TextFormat("mode: %s", mode_name(s.mode)),
                s.mode == InputMode::TOUCH ? GREEN :
                s.mode == InputMode::MOUSE ? SKYBLUE : LIGHTGRAY);
            line(TextFormat("mouse: %.0f, %.0f  L:%s R:%s",
                s.mouse_x, s.mouse_y,
                s.mouse_left ? "DN" : "--", s.mouse_right ? "DN" : "--"));
            line(TextFormat("wheel: %.1f, %.1f  accum: %.1f", s.wheel_x, s.wheel_y, s.wheel_accum));
            line(TextFormat("touch count: %d", s.touch_count), s.touch_count > 0 ? GREEN : LIGHTGRAY);
            line(TextFormat("gesture: %s  (last: %s)",
                gesture_name(s.gesture), gesture_name(s.last_gesture)),
                s.gesture != GESTURE_NONE ? ORANGE : LIGHTGRAY);
            if (s.last_gesture == GESTURE_DRAG)
                line(TextFormat("  drag: %.2f, %.2f", s.drag_x, s.drag_y), ORANGE);
            if (s.last_gesture == GESTURE_PINCH_IN || s.last_gesture == GESTURE_PINCH_OUT)
                line(TextFormat("  pinch angle: %.1f", s.pinch_angle), ORANGE);
            line(TextFormat("last key: %s", s.last_key_name),
                s.last_key > 0 ? MAROON : LIGHTGRAY);
            line(TextFormat("arrows: %s%s%s%s  space:%s",
                s.key_up ? "U" : "-", s.key_down ? "D" : "-",
                s.key_left ? "L" : "-", s.key_right ? "R" : "-",
                s.key_space ? "Y" : "-"), LIGHTGRAY);

            // render visuals based on input mode
            if (s.mode == InputMode::TOUCH) {
                for (int i = 0; i < s.touch_count && i < 10; i++) {
                    Vector2 tp = GetTouchPosition(i);
                    DrawCircleV(tp, 16.0f, Fade(GREEN, 0.5f));
                    DrawText(TextFormat("t%d", i), (int)tp.x + 20, (int)tp.y - 8, 14, GREEN);
                }
            } else if (s.mode == InputMode::MOUSE) {
                DrawLine((int)s.mouse_x - 20, (int)s.mouse_y,
                         (int)s.mouse_x + 20, (int)s.mouse_y, BLUE);
                DrawLine((int)s.mouse_x, (int)s.mouse_y - 20,
                         (int)s.mouse_x, (int)s.mouse_y + 20, BLUE);
                if (s.mouse_left)
                    DrawCircleV({s.mouse_x, s.mouse_y}, 12.0f, Fade(RED, 0.4f));
                if (s.mouse_right)
                    DrawCircleV({s.mouse_x, s.mouse_y}, 12.0f, Fade(PURPLE, 0.4f));
            }
            // IDLE: nothing rendered -- waiting for input source
        });

    ecs.app().run();
    return 0;
}
