#pragma once

#include <raylib.h>
#include <raymath.h>

namespace graphics {

// Forward declare detail namespace for camera reference
namespace detail {
    inline Camera3D camera{};
}

// Camera configuration - all fields have sensible defaults
struct CameraConfig {
    Vector3 position = {5.0f, 5.0f, 5.0f};
    Vector3 target = {0.0f, 0.0f, 0.0f};
    Vector3 up = {0.0f, 1.0f, 0.0f};
    float fovy = 60.0f;
    int projection = CAMERA_PERSPECTIVE;
};

// Control speeds for camera movement
struct CameraControlConfig {
    float move_speed = 0.1f;
    float rotation_speed = 0.2f;
    float zoom_speed = 1.0f;
};

// Default configs as constexpr
inline constexpr CameraConfig default_camera_config{
    {5.0f, 5.0f, 5.0f},  // position
    {0.0f, 0.0f, 0.0f},  // target
    {0.0f, 1.0f, 0.0f},  // up
    60.0f,               // fovy
    CAMERA_PERSPECTIVE   // projection
};

inline constexpr CameraControlConfig default_control_config{
    0.1f,  // move_speed
    0.2f,  // rotation_speed
    1.0f   // zoom_speed
};

// Initialize camera with config
inline void init_camera(const CameraConfig& config = default_camera_config) {
    auto& cam = detail::camera;
    cam.position = config.position;
    cam.target = config.target;
    cam.up = config.up;
    cam.fovy = config.fovy;
    cam.projection = config.projection;
}

// Update camera with WASD + mouse drag + scroll wheel controls
// WASD/QE: move forward/back/left/right/up/down
// Mouse drag (left button): rotate
// Scroll wheel: zoom
inline void update_camera_controls(const CameraControlConfig& config = default_control_config) {
    UpdateCameraPro(
        &detail::camera,
        // Movement: WASD for horizontal, QE for vertical
        {
            static_cast<float>(IsKeyDown(KEY_W) - IsKeyDown(KEY_S)) * config.move_speed,
            static_cast<float>(IsKeyDown(KEY_D) - IsKeyDown(KEY_A)) * config.move_speed,
            static_cast<float>(IsKeyDown(KEY_E) - IsKeyDown(KEY_Q)) * config.move_speed
        },
        // Rotation: mouse drag or arrow keys
        {
            GetMouseDelta().x * static_cast<float>(IsMouseButtonDown(MOUSE_LEFT_BUTTON)) * config.rotation_speed +
                static_cast<float>(IsKeyDown(KEY_RIGHT) - IsKeyDown(KEY_LEFT)) * 1.5f,
            GetMouseDelta().y * static_cast<float>(IsMouseButtonDown(MOUSE_LEFT_BUTTON)) * config.rotation_speed +
                static_cast<float>(IsKeyDown(KEY_DOWN) - IsKeyDown(KEY_UP)) * 1.5f,
            0.0f
        },
        // Zoom: mouse wheel
        GetMouseWheelMove() * -config.zoom_speed
    );
}

// Get current camera (mutable reference for direct manipulation)
[[nodiscard]] inline Camera3D& get_camera() {
    return detail::camera;
}

// Get current camera (const reference for read-only access)
[[nodiscard]] inline const Camera3D& get_camera_const() {
    return detail::camera;
}

} // namespace graphics
