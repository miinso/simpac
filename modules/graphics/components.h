#pragma once

#include <flecs.h>
#include <raylib.h>

#include "types.h"
#include "tags.h"

namespace graphics {

struct Camera {
    vec3f target = {0.0f, 0.0f, 0.0f};
    vec3f up = {0.0f, 1.0f, 0.0f};
    float fovy = 60.0f;
    int projection = 0;  // 0 = CAMERA_PERSPECTIVE, 1 = CAMERA_ORTHOGRAPHIC

    float move_speed = 0.1f;
    float rotation_speed = 0.2f;
    float zoom_speed = 1.0f;

    // runtime touch tracking (not serialized)
    float prev_touch_x = 0, prev_touch_y = 0;
    int prev_touch_count = 0;
    bool was_touching = false;
};

struct ActiveCamera {};

struct color4f : engine::vec4<color4f, Real> {
    using Base = engine::vec4<color4f, Real>;
    color4f() : Base((Real)1.0, (Real)1.0, (Real)1.0, (Real)1.0) {}
    color4f(float r_, float g_, float b_, float a_)
        : Base((Real)r_, (Real)g_, (Real)b_, (Real)a_) {}
    using Base::vec4;

    static inline void meta(flecs::world& ecs) {
        ecs.component<color4f>("color4f")
            .member("r", &color4f::x)
            .member("g", &color4f::y)
            .member("b", &color4f::z)
            .member("a", &color4f::w);
    }
};

[[nodiscard]] inline color4f to_rgba(Color color) {
    return {
        color.r / 255.0f,
        color.g / 255.0f,
        color.b / 255.0f,
        color.a / 255.0f
    };
}

[[nodiscard]] inline Color to_color(const color4f& color) {
    auto to_byte = [](Real v) -> unsigned char {
        Real clamped = v < (Real)0 ? (Real)0 : (v > (Real)1 ? (Real)1 : v);
        return (unsigned char)(clamped * 255.0f);
    };
    return {to_byte(color.x), to_byte(color.y), to_byte(color.z), to_byte(color.w)};
}

// renderable model reference — attach to entities for query-based rendering
// NOTE: requires Position component for rendering/shadow queries
struct ModelRef {
    Model* model = nullptr;
    float scale = 1.0f;
    float tiling[2] = {0.5f, 0.5f};
};

// tag: entity casts shadows (used as filter on shadow pass queries)
struct ShadowCaster {};

namespace components {

inline void register_camera_components(flecs::world& ecs) {
    engine::register_vec3_component<vec3f>(ecs, "vec3f");
    engine::register_vec3_component<vec3d>(ecs, "vec3d");
    engine::register_vec3_component<vec3r>(ecs, "vec3r");
    engine::register_vec3_component<Position>(ecs);

    ecs.component<Camera>("graphics::Camera")
        .member<vec3f>("target")
        .member<vec3f>("up")
        .member<float>("fovy")
        .member<int>("projection")
        .member<float>("move_speed")
        .member<float>("rotation_speed")
        .member<float>("zoom_speed");

    ecs.component<ActiveCamera>("graphics::ActiveCamera")
        .add(flecs::Exclusive)
        .add(flecs::PairIsTag);
}

inline void register_components(flecs::world& world) {
    color4f::meta(world);
    world.component<Configurable>();
    world.component<ModelRef>()
        .member<float>("scale");
    world.component<ShadowCaster>();
}

} // namespace components
} // namespace graphics
