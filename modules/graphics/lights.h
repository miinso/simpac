#pragma once

#include <flecs.h>

#include "components.h"

namespace graphics {

// max lights supported by shader
inline constexpr int MAX_LIGHTS = 8;

// light type indices (match shader defines)
inline constexpr int LIGHT_DIRECTIONAL = 0;
inline constexpr int LIGHT_POINT       = 1;
inline constexpr int LIGHT_SPOT        = 2;

struct LightRenderer; // forward decl for upload methods

// --- typed light components ---

struct AmbientLight {
    color4f color = {0.1f, 0.1f, 0.1f, 1.0f};
    float intensity = 1.0f;

    void upload(const LightRenderer& lr) const;
};

struct DirectionalLight {
    bool enabled = true;
    color4f color = {1, 1, 1, 1};
    float intensity = 1.0f;
    vec3f direction = {0, -1, 0};

    void upload(const LightRenderer& lr, int index) const;
};

struct PointLight {
    bool enabled = true;
    color4f color = {1, 1, 1, 1};
    float intensity = 1.0f;
    float range = 10.0f;
    float falloff = 2.0f;

    void upload(const LightRenderer& lr, int index, const Position& pos) const;
};

struct SpotLight {
    bool enabled = true;
    color4f color = {1, 1, 1, 1};
    float intensity = 1.0f;
    vec3f direction = {0, -1, 0};
    float range = 10.0f;
    float falloff = 2.0f;
    float inner_angle = 30.0f;  // degrees
    float outer_angle = 45.0f;  // degrees

    void upload(const LightRenderer& lr, int index, const Position& pos) const;
};

namespace components {

inline void register_light_components(flecs::world& world) {
    world.component<AmbientLight>()
        .member<color4f>("color")
        .member<float>("intensity");

    world.component<DirectionalLight>()
        .member<bool>("enabled")
        .member<color4f>("color")
        .member<float>("intensity")
        .member<vec3f>("direction");

    world.component<PointLight>()
        .member<bool>("enabled")
        .member<color4f>("color")
        .member<float>("intensity")
        .member<float>("range")
        .member<float>("falloff");

    world.component<SpotLight>()
        .member<bool>("enabled")
        .member<color4f>("color")
        .member<float>("intensity")
        .member<vec3f>("direction")
        .member<float>("range")
        .member<float>("falloff")
        .member<float>("inner_angle")
        .member<float>("outer_angle");
}

} // namespace components

} // namespace graphics
