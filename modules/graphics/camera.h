#pragma once

#include <flecs.h>
#include <raylib.h>
#include <raymath.h>

namespace graphics {

// ============================================================================
// Camera Component - state + controls in one
// ============================================================================

struct Camera {
    // State
    float position[3] = {5.0f, 5.0f, 5.0f};
    float target[3] = {0.0f, 0.0f, 0.0f};
    float up[3] = {0.0f, 1.0f, 0.0f};
    float fovy = 60.0f;
    int projection = 0;  // 0 = CAMERA_PERSPECTIVE, 1 = CAMERA_ORTHOGRAPHIC

    // Controls
    float move_speed = 0.1f;
    float rotation_speed = 0.2f;
    float zoom_speed = 1.0f;
    bool controls_enabled = true;
};

// Tag: which camera is used for rendering
struct ActiveCamera {};

// ============================================================================
// Internal state - raylib Camera3D synced from active Camera component
// ============================================================================

namespace detail {
    inline Camera3D raylib_camera{};
}

// ============================================================================
// Component registration with reflection (for REST API)
// ============================================================================

inline void register_camera_components(flecs::world& ecs) {
    ecs.component<Camera>()
        .member<float>("position", 3)
        .member<float>("target", 3)
        .member<float>("up", 3)
        .member<float>("fovy")
        .member<int>("projection")
        .member<float>("move_speed")
        .member<float>("rotation_speed")
        .member<float>("zoom_speed")
        .member<bool>("controls_enabled");

    ecs.component<ActiveCamera>();
}

// ============================================================================
// Conversion helpers
// ============================================================================

inline Camera3D to_raylib(const Camera& cam) {
    Camera3D rc{};
    rc.position = {cam.position[0], cam.position[1], cam.position[2]};
    rc.target = {cam.target[0], cam.target[1], cam.target[2]};
    rc.up = {cam.up[0], cam.up[1], cam.up[2]};
    rc.fovy = cam.fovy;
    rc.projection = cam.projection;
    return rc;
}

inline void from_raylib(Camera& cam, const Camera3D& rc) {
    cam.position[0] = rc.position.x;
    cam.position[1] = rc.position.y;
    cam.position[2] = rc.position.z;
    cam.target[0] = rc.target.x;
    cam.target[1] = rc.target.y;
    cam.target[2] = rc.target.z;
    cam.up[0] = rc.up.x;
    cam.up[1] = rc.up.y;
    cam.up[2] = rc.up.z;
    cam.fovy = rc.fovy;
    cam.projection = rc.projection;
}

// ============================================================================
// Camera control update (called by system)
// ============================================================================

inline void update_camera_controls(Camera& cam) {
    if (!cam.controls_enabled) return;

    // Convert to raylib, apply controls, convert back
    Camera3D rc = to_raylib(cam);

    UpdateCameraPro(
        &rc,
        // Movement: WASD for horizontal, QE for vertical
        {
            static_cast<float>(IsKeyDown(KEY_W) - IsKeyDown(KEY_S)) * cam.move_speed,
            static_cast<float>(IsKeyDown(KEY_D) - IsKeyDown(KEY_A)) * cam.move_speed,
            static_cast<float>(IsKeyDown(KEY_E) - IsKeyDown(KEY_Q)) * cam.move_speed
        },
        // Rotation: mouse drag or arrow keys
        {
            GetMouseDelta().x * static_cast<float>(IsMouseButtonDown(MOUSE_LEFT_BUTTON)) * cam.rotation_speed +
                static_cast<float>(IsKeyDown(KEY_RIGHT) - IsKeyDown(KEY_LEFT)) * 1.5f,
            GetMouseDelta().y * static_cast<float>(IsMouseButtonDown(MOUSE_LEFT_BUTTON)) * cam.rotation_speed +
                static_cast<float>(IsKeyDown(KEY_DOWN) - IsKeyDown(KEY_UP)) * 1.5f,
            0.0f
        },
        // Zoom: mouse wheel
        GetMouseWheelMove() * -cam.zoom_speed
    );

    from_raylib(cam, rc);
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

} // namespace graphics
