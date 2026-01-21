#pragma once

#include <Eigen/Dense>
#include <flecs.h>
#include <raylib.h>
#include <raymath.h>

#include <cstring>

namespace graphics {

// ============================================================================
// Camera Component - state + controls in one
// ============================================================================

struct Camera {
    // state
    // float position[3] = {5.0f, 5.0f, 5.0f};
    Eigen::Vector3f target = Eigen::Vector3f(0.0f, 0.0f, 0.0f);
    Eigen::Vector3f up = Eigen::Vector3f(0.0f, 1.0f, 0.0f);
    float fovy = 60.0f;
    int projection = 0;  // 0 = CAMERA_PERSPECTIVE, 1 = CAMERA_ORTHOGRAPHIC

    // controls
    float move_speed = 0.1f;
    float rotation_speed = 0.2f;
    float zoom_speed = 1.0f;
    bool controls_enabled = true;
};

// position component for camera entities
struct Position {
    Eigen::Vector3f value = Eigen::Vector3f(5.0f, 5.0f, 5.0f);
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

inline void register_vector3f_reflection(flecs::world& ecs) {
    auto vec3f_meta = ecs.component()
        .member<float>("x")
        .member<float>("y")
        .member<float>("z");

    ecs.component<Eigen::Vector3f>()
        .opaque(vec3f_meta)
        .serialize([](const flecs::serializer* s, const Eigen::Vector3f* data) {
            if (!data) return 0;
            const float* v = data->data();
            s->member("x");
            s->value(v[0]);
            s->member("y");
            s->value(v[1]);
            s->member("z");
            s->value(v[2]);
            return 0;
        })
        .ensure_member([](Eigen::Vector3f* dst, const char* member) -> void* {
            if (!dst || !member) return nullptr;
            float* v = dst->data();
            if (!std::strcmp(member, "x")) return &v[0];
            if (!std::strcmp(member, "y")) return &v[1];
            if (!std::strcmp(member, "z")) return &v[2];
            return nullptr;
        });
}

inline void register_camera_components(flecs::world& ecs) {
    register_vector3f_reflection(ecs);

    ecs.component<Position>()
        .member<Eigen::Vector3f>("value");

    ecs.component<Camera>()
        .member<Eigen::Vector3f>("target")
        .member<Eigen::Vector3f>("up")
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

inline Camera3D to_raylib(const Position& pos, const Camera& cam) {
    Camera3D rc{};
    rc.position = {pos.value.x(), pos.value.y(), pos.value.z()};
    rc.target = {cam.target.x(), cam.target.y(), cam.target.z()};
    rc.up = {cam.up.x(), cam.up.y(), cam.up.z()};
    rc.fovy = cam.fovy;
    rc.projection = cam.projection;
    return rc;
}

inline void from_raylib(Position& pos, Camera& cam, const Camera3D& rc) {
    pos.value = Eigen::Vector3f(rc.position.x, rc.position.y, rc.position.z);
    cam.target = Eigen::Vector3f(rc.target.x, rc.target.y, rc.target.z);
    cam.up = Eigen::Vector3f(rc.up.x, rc.up.y, rc.up.z);
    cam.fovy = rc.fovy;
    cam.projection = rc.projection;
}

// ============================================================================
// Camera control update (called by system)
// ============================================================================

inline void update_camera_controls(Position& pos, Camera& cam) {
    if (!cam.controls_enabled) return;

    // Convert to raylib, apply controls, convert back
    Camera3D rc = to_raylib(pos, cam);

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

} // namespace graphics
