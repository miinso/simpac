#pragma once

#include <flecs.h>
#include <raylib.h>
#include <raymath.h>
#include <Eigen/Dense>

#include <cstddef>

#include "input.h"

namespace graphics {

// ============================================================================
// Camera Component - state + controls in one
// ============================================================================

template <typename Derived>
struct vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    vec3() = default;
    vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

    float* data() { return &x; }
    const float* data() const { return &x; }

    float& operator[](size_t i) { return data()[i]; }
    const float& operator[](size_t i) const { return data()[i]; }
};

struct vec3f : vec3<vec3f> {
    using vec3<vec3f>::vec3;
};

struct Camera {
    // state
    // float position[3] = {5.0f, 5.0f, 5.0f};
    vec3f target = {0.0f, 0.0f, 0.0f};
    vec3f up = {0.0f, 1.0f, 0.0f};
    float fovy = 60.0f;
    int projection = 0;  // 0 = CAMERA_PERSPECTIVE, 1 = CAMERA_ORTHOGRAPHIC

    // controls
    float move_speed = 0.1f;
    float rotation_speed = 0.2f;
    float zoom_speed = 1.0f;
    // bool controls_enabled = true;
};

// TODO: we could make a slot for (optional) target entity so that
// camera picks up `Position` component of the target (if exists)

// position component for camera entities
struct Position : vec3<Position> {
    Position() : vec3<Position>(5.0f, 5.0f, 5.0f) {}
    using vec3<Position>::vec3;
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

namespace components {

template <typename T>
inline void register_vec3f(flecs::world& ecs) {
    ecs.component<T>()
        .template member<float>("x")
        .template member<float>("y")
        .template member<float>("z");
}

inline void register_camera_components(flecs::world& ecs) {
    register_vec3f<vec3f>(ecs);
    register_vec3f<Position>(ecs);

    ecs.component<Camera>()
        .member<vec3f>("target")
        .member<vec3f>("up")
        .member<float>("fovy")
        .member<int>("projection")
        .member<float>("move_speed")
        .member<float>("rotation_speed")
        .member<float>("zoom_speed");
        // .member<bool>("controls_enabled");

    ecs.component<ActiveCamera>();
}

} // namespace components

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

// remove active tag from all camera entities
inline void clear_active(flecs::world& ecs) {
    ecs.query_builder<Camera>()
        .with<ActiveCamera>()
        .build()
        .each([](flecs::entity e, Camera&) {
            e.remove<ActiveCamera>();
        });
}

// make sure one camera is active when none exists
inline void ensure_default(flecs::world& ecs) {
    auto active_camera_query = ecs.query<Camera, ActiveCamera>();
    if (active_camera_query.count() > 0) return;

    ecs.entity("DefaultCamera")
        .set<Camera>({})
        .set<Position>({})
        .add<ActiveCamera>();
}

// create camera entity from explicit position + camera params
inline flecs::entity create(flecs::world& ecs, const char* name, const Position& pos, const Camera& cam = {}, bool make_active = false) {
    auto entity = ecs.entity(name)
        .set<Camera>(cam)
        .set<Position>(pos);

    if (make_active) {
        clear_active(ecs);
        entity.add<ActiveCamera>();
    }
    return entity;
}

// create camera entity with default position
inline flecs::entity create(flecs::world& ecs, const char* name, const Camera& cam = {}, bool make_active = false) {
    return create(ecs, name, Position{}, cam, make_active);
}

// set given camera as the single active camera
inline void set_active(flecs::world& ecs, flecs::entity camera_entity) {
    if (!camera_entity.has<Camera>()) return;
    clear_active(ecs);
    camera_entity.add<ActiveCamera>();
}

} // namespace camera

} // namespace graphics
