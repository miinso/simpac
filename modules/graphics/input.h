#pragma once

#include <raylib.h>
#include <raymath.h>

// input shim (since Raylib running in Worker can't access DOM input)
namespace graphics::shim {
    // shared state (written by platform WASM, read by game)
    inline Vector2 mouse_pos = {0.0f, 0.0f};
    inline Vector2 mouse_delta = {0.0f, 0.0f};
    inline float mouse_wheel = 0.0f;
    inline bool mouse_btn[8] = {false};
    inline bool prev_mouse_btn[8] = {false};
    inline bool keys[512] = {false};
    inline bool prev_keys[512] = {false};
    inline bool mouse_in_canvas = true;

    inline void update() {
        for (int i = 0; i < 8; ++i)
            prev_mouse_btn[i] = mouse_btn[i];
        for (int i = 0; i < 512; ++i)
            prev_keys[i] = keys[i];
        mouse_delta = {0.0f, 0.0f};
        // mouse_wheel handled by getter reset
    }

    // proxy functions
    inline Vector2 GetMousePosition() { return mouse_pos; }
    inline Vector2 GetMouseDelta() { return mouse_delta; }
    inline float GetMouseWheelMove() {
        float w = mouse_wheel;
        mouse_wheel = 0;
        return w;
    }

    inline bool IsMouseButtonDown(int button) {
        return (button >= 0 && button < 8) && mouse_btn[button];
    }

    inline bool IsMouseButtonPressed(int button) {
        return (button >= 0 && button < 8) && mouse_btn[button] && !prev_mouse_btn[button];
    }

    inline bool IsMouseButtonReleased(int button) {
        return (button >= 0 && button < 8) && !mouse_btn[button] && prev_mouse_btn[button];
    }

    inline bool IsMouseButtonUp(int button) {
        return (button >= 0 && button < 8) && !mouse_btn[button];
    }

    inline bool IsKeyDown(int key) { return (key >= 0 && key < 512) && keys[key]; }

    inline bool IsKeyPressed(int key) {
        return (key >= 0 && key < 512) && keys[key] && !prev_keys[key];
    }

    inline bool IsKeyReleased(int key) {
        return (key >= 0 && key < 512) && !keys[key] && prev_keys[key];
    }

    inline bool IsCursorOnScreen() { return mouse_in_canvas; }

    // touch Shim
    struct TouchPoint {
        int id = -1;
        Vector2 pos = {0, 0};
        bool active = false;
    };
    inline TouchPoint touches[10];
    inline int touch_count = 0;

    inline int GetTouchPointCount() { return touch_count; }
    inline Vector2 GetTouchPosition(int index) {
        if (index >= 0 && index < 10 && touches[index].active)
            return touches[index].pos;
        return {0, 0};
    }

    // cursor control (implemented in platform cpp)
    void show_cursor(bool show);

} // namespace graphics::shim

namespace graphics::input {
    // when true, camera/input consumers should ignore left-mouse drag this frame.
    inline bool capture_mouse_left = false;
}

// override raylib functions with our shim
#if defined(__EMSCRIPTEN__)
// undefine just in case (though usually functions aren't macros)
#define GetMousePosition graphics::shim::GetMousePosition
#define GetMouseDelta graphics::shim::GetMouseDelta
#define GetMouseWheelMove graphics::shim::GetMouseWheelMove
#define IsMouseButtonDown graphics::shim::IsMouseButtonDown
#define IsMouseButtonPressed graphics::shim::IsMouseButtonPressed
#define IsMouseButtonReleased graphics::shim::IsMouseButtonReleased
#define IsMouseButtonUp graphics::shim::IsMouseButtonUp
#define IsKeyDown graphics::shim::IsKeyDown
#define IsKeyPressed graphics::shim::IsKeyPressed
#define IsKeyReleased graphics::shim::IsKeyReleased
#define IsCursorOnScreen graphics::shim::IsCursorOnScreen

#define GetTouchPointCount graphics::shim::GetTouchPointCount
#define GetTouchPosition graphics::shim::GetTouchPosition

// cursors
#define ShowCursor() graphics::shim::show_cursor(true)
#define HideCursor() graphics::shim::show_cursor(false)
#define EnableCursor() graphics::shim::show_cursor(true)
#define DisableCursor() graphics::shim::show_cursor(false)
#endif
