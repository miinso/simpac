#pragma once

#include <flecs.h>
#include <raylib.h>
#include <raymath.h>

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
    // if (!cam.controls_enabled) return;

    // Convert to raylib, apply controls, convert back
    Camera3D rc = to_raylib(pos, cam);

    UpdateCameraPro(
        &rc,
        // Movement: WASD for horizontal, QE for vertical
        {
            (IsKeyDown(KEY_W) - IsKeyDown(KEY_S)) * cam.move_speed,
            (IsKeyDown(KEY_D) - IsKeyDown(KEY_A)) * cam.move_speed,
            (IsKeyDown(KEY_E) - IsKeyDown(KEY_Q)) * cam.move_speed
        },
        // Rotation: mouse drag or arrow keys
        {
            GetMouseDelta().x * (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && !input::capture_mouse_left) * cam.rotation_speed +
                (IsKeyDown(KEY_RIGHT) - IsKeyDown(KEY_LEFT)) * 1.5f,
            GetMouseDelta().y * (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && !input::capture_mouse_left) * cam.rotation_speed +
                (IsKeyDown(KEY_DOWN) - IsKeyDown(KEY_UP)) * 1.5f,
            0.0f
        },
        // Zoom: mouse wheel
        GetMouseWheelMove() * -cam.zoom_speed
    );

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
