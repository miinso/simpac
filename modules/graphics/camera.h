#pragma once

#include <flecs.h>
#include <raylib.h>
#include <raymath.h>
#include <rcamera.h>

#include "components.h"
#include "input.h"

namespace graphics {

// ============================================================================
// Camera Component - state + controls in one
// ============================================================================
// ============================================================================
// Internal state - raylib Camera3D synced from active Camera component
// ============================================================================

namespace detail {
    inline Camera3D raylib_camera{};
}

// ============================================================================
// Conversion helpers
// ============================================================================

inline Camera3D to_raylib(const Position& pos, const Camera& cam) {
    Camera3D rc{};
    rc.position = {pos.x, pos.y, pos.z};
    rc.target = {cam.target.x, cam.target.y, cam.target.z};
    rc.up = {cam.up.x, cam.up.y, cam.up.z};
    rc.fovy = cam.fovy;
    rc.projection = cam.projection;
    return rc;
}

inline void from_raylib(Position& pos, Camera& cam, const Camera3D& rc) {
    pos.x = rc.position.x;
    pos.y = rc.position.y;
    pos.z = rc.position.z;
    cam.target.x = rc.target.x;
    cam.target.y = rc.target.y;
    cam.target.z = rc.target.z;
    cam.up.x = rc.up.x;
    cam.up.y = rc.up.y;
    cam.up.z = rc.up.z;
    cam.fovy = rc.fovy;
    cam.projection = rc.projection;
}

// ============================================================================
// Camera control update (called by system)
// ============================================================================

inline void update_camera_controls(Position& pos, Camera& cam) {
    Camera3D rc = to_raylib(pos, cam);

    int touch_count = GetTouchPointCount();
    bool touching = touch_count > 0;

    if (touching) {
        // touch: single-finger drag orbits, pinch zooms
        Vector2 tp = GetTouchPosition(0);
        if (!cam.was_touching || cam.prev_touch_count != touch_count) {
            cam.prev_touch_x = tp.x;
            cam.prev_touch_y = tp.y;
        } else if (touch_count == 1) {
            float dx = tp.x - cam.prev_touch_x;
            float dy = tp.y - cam.prev_touch_y;
            CameraYaw(&rc, -dx * cam.rotation_speed * DEG2RAD, true);
            CameraPitch(&rc, -dy * cam.rotation_speed * DEG2RAD, true, true, false);
            cam.prev_touch_x = tp.x;
            cam.prev_touch_y = tp.y;
        }
        int gesture = GetGestureDetected();
        if (gesture == GESTURE_PINCH_IN)
            CameraMoveToTarget(&rc, cam.zoom_speed);
        else if (gesture == GESTURE_PINCH_OUT)
            CameraMoveToTarget(&rc, -cam.zoom_speed);
    } else {
        // mouse: left-drag orbits, wheel zooms
        // IsMouseButtonDown is never true from touch (raylib only sets it from
        // actual mouse button callbacks), so this branch is touch-safe
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && !input::capture_mouse_left) {
            Vector2 delta = GetMouseDelta();
            CameraYaw(&rc, -delta.x * cam.rotation_speed * DEG2RAD, true);
            CameraPitch(&rc, -delta.y * cam.rotation_speed * DEG2RAD, true, true, false);
        }
        float wheel = GetMouseWheelMove();
        if (wheel != 0.0f)
            CameraMoveToTarget(&rc, wheel * -cam.zoom_speed);
    }

    // arrow keys (always active)
    float key_yaw = (float)(IsKeyDown(KEY_RIGHT) - IsKeyDown(KEY_LEFT)) * 1.5f;
    float key_pitch = (float)(IsKeyDown(KEY_DOWN) - IsKeyDown(KEY_UP)) * 1.5f;
    if (key_yaw != 0.0f || key_pitch != 0.0f) {
        CameraYaw(&rc, -key_yaw * DEG2RAD, true);
        CameraPitch(&rc, -key_pitch * DEG2RAD, true, true, false);
    }

    // WASD movement (works with both touch and mouse)
    CameraMoveForward(&rc, (float)(IsKeyDown(KEY_W) - IsKeyDown(KEY_S)) * cam.move_speed, true);
    CameraMoveRight(&rc, (float)(IsKeyDown(KEY_D) - IsKeyDown(KEY_A)) * cam.move_speed, true);
    CameraMoveUp(&rc, (float)(IsKeyDown(KEY_E) - IsKeyDown(KEY_Q)) * cam.move_speed);

    cam.was_touching = touching;
    cam.prev_touch_count = touch_count;
    from_raylib(pos, cam, rc);
}

// ============================================================================
// Accessors
// ============================================================================

// Get the synced raylib camera (updated each frame from active Camera)
[[nodiscard]] inline Camera3D& get_raylib_camera() {
    return detail::raylib_camera;
}

[[nodiscard]] inline const Camera3D& get_raylib_camera_const() {
    return detail::raylib_camera;
}

namespace camera {

inline flecs::entity active_camera_source(flecs::world& ecs) {
    return ecs.entity("graphics.ActiveCameraSource");
}

} // namespace camera

} // namespace graphics
