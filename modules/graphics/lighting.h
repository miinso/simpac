#pragma once

#include <raylib.h>
#include <flecs.h>
#include <cmath>

#include "lights.h"
#include "materials.h"

namespace graphics {

// singleton managing shader + uniform loc cache
struct LightRenderer {
    Shader shader = {};
    int view_pos_loc = -1;
    int ambient_color_loc = -1;
    int ambient_intensity_loc = -1;
    int num_lights_loc = -1;

    // per-light uniform loc cache
    struct LightLocs {
        int enabled = -1, type = -1, position = -1;
        int direction = -1, color = -1, intensity = -1;
        int range = -1, falloff = -1;
        int cos_inner = -1, cos_outer = -1;
    };
    LightLocs light_locs[MAX_LIGHTS] = {};

    // cached material uniform locs
    MaterialLocs mat_locs = {};

    // shader source paths (null = use default)
    const char* vs_path = nullptr;
    const char* fs_path = nullptr;

    void upload_camera_pos(const vec3f& cam_pos) const {
        if (!shader.id) return;
        float pos[3] = {cam_pos.x, cam_pos.y, cam_pos.z};
        SetShaderValue(shader, view_pos_loc, pos, SHADER_UNIFORM_VEC3);
    }

    void upload_light_count(int count) const {
        if (!shader.id) return;
        SetShaderValue(shader, num_lights_loc, &count, SHADER_UNIFORM_INT);
    }
};

// --- light upload definitions (declared in lights.h) ---

inline void AmbientLight::upload(const LightRenderer& lr) const {
    if (!lr.shader.id) return;
    float col[3] = {color.x, color.y, color.z};
    SetShaderValue(lr.shader, lr.ambient_color_loc, col, SHADER_UNIFORM_VEC3);
    SetShaderValue(lr.shader, lr.ambient_intensity_loc, &intensity, SHADER_UNIFORM_FLOAT);
}

inline void DirectionalLight::upload(const LightRenderer& lr, int index) const {
    if (!lr.shader.id || index < 0 || index >= MAX_LIGHTS) return;
    const auto& loc = lr.light_locs[index];

    int en = enabled ? 1 : 0;
    int tp = LIGHT_DIRECTIONAL;
    SetShaderValue(lr.shader, loc.enabled, &en, SHADER_UNIFORM_INT);
    SetShaderValue(lr.shader, loc.type, &tp, SHADER_UNIFORM_INT);

    float pos[3] = {0, 0, 0};
    SetShaderValue(lr.shader, loc.position, pos, SHADER_UNIFORM_VEC3);

    float dir[3] = {direction.x, direction.y, direction.z};
    SetShaderValue(lr.shader, loc.direction, dir, SHADER_UNIFORM_VEC3);

    float col[4] = {color.x, color.y, color.z, color.w};
    SetShaderValue(lr.shader, loc.color, col, SHADER_UNIFORM_VEC4);
    SetShaderValue(lr.shader, loc.intensity, &intensity, SHADER_UNIFORM_FLOAT);

    float zero = 0.0f;
    SetShaderValue(lr.shader, loc.range, &zero, SHADER_UNIFORM_FLOAT);
    SetShaderValue(lr.shader, loc.falloff, &zero, SHADER_UNIFORM_FLOAT);
    SetShaderValue(lr.shader, loc.cos_inner, &zero, SHADER_UNIFORM_FLOAT);
    SetShaderValue(lr.shader, loc.cos_outer, &zero, SHADER_UNIFORM_FLOAT);
}

inline void PointLight::upload(const LightRenderer& lr, int index, const Position& pos) const {
    if (!lr.shader.id || index < 0 || index >= MAX_LIGHTS) return;
    const auto& loc = lr.light_locs[index];

    int en = enabled ? 1 : 0;
    int tp = LIGHT_POINT;
    SetShaderValue(lr.shader, loc.enabled, &en, SHADER_UNIFORM_INT);
    SetShaderValue(lr.shader, loc.type, &tp, SHADER_UNIFORM_INT);

    float p[3] = {pos.x, pos.y, pos.z};
    SetShaderValue(lr.shader, loc.position, p, SHADER_UNIFORM_VEC3);

    float dir[3] = {0, 0, 0};
    SetShaderValue(lr.shader, loc.direction, dir, SHADER_UNIFORM_VEC3);

    float col[4] = {color.x, color.y, color.z, color.w};
    SetShaderValue(lr.shader, loc.color, col, SHADER_UNIFORM_VEC4);
    SetShaderValue(lr.shader, loc.intensity, &intensity, SHADER_UNIFORM_FLOAT);
    SetShaderValue(lr.shader, loc.range, &range, SHADER_UNIFORM_FLOAT);
    SetShaderValue(lr.shader, loc.falloff, &falloff, SHADER_UNIFORM_FLOAT);

    float zero = 0.0f;
    SetShaderValue(lr.shader, loc.cos_inner, &zero, SHADER_UNIFORM_FLOAT);
    SetShaderValue(lr.shader, loc.cos_outer, &zero, SHADER_UNIFORM_FLOAT);
}

inline void SpotLight::upload(const LightRenderer& lr, int index, const Position& pos) const {
    if (!lr.shader.id || index < 0 || index >= MAX_LIGHTS) return;
    const auto& loc = lr.light_locs[index];

    int en = enabled ? 1 : 0;
    int tp = LIGHT_SPOT;
    SetShaderValue(lr.shader, loc.enabled, &en, SHADER_UNIFORM_INT);
    SetShaderValue(lr.shader, loc.type, &tp, SHADER_UNIFORM_INT);

    float p[3] = {pos.x, pos.y, pos.z};
    SetShaderValue(lr.shader, loc.position, p, SHADER_UNIFORM_VEC3);

    float dir[3] = {direction.x, direction.y, direction.z};
    SetShaderValue(lr.shader, loc.direction, dir, SHADER_UNIFORM_VEC3);

    float col[4] = {color.x, color.y, color.z, color.w};
    SetShaderValue(lr.shader, loc.color, col, SHADER_UNIFORM_VEC4);
    SetShaderValue(lr.shader, loc.intensity, &intensity, SHADER_UNIFORM_FLOAT);
    SetShaderValue(lr.shader, loc.range, &range, SHADER_UNIFORM_FLOAT);
    SetShaderValue(lr.shader, loc.falloff, &falloff, SHADER_UNIFORM_FLOAT);

    float ci = cosf(inner_angle * 3.14159265f / 180.0f);
    float co = cosf(outer_angle * 3.14159265f / 180.0f);
    SetShaderValue(lr.shader, loc.cos_inner, &ci, SHADER_UNIFORM_FLOAT);
    SetShaderValue(lr.shader, loc.cos_outer, &co, SHADER_UNIFORM_FLOAT);
}

namespace queries {

inline flecs::query<const AmbientLight> ambient;
inline flecs::query<const DirectionalLight> directional;
// NOTE: point/spot queries require Position — entities without it are silently excluded
inline flecs::query<const PointLight, const Position> point;
inline flecs::query<const SpotLight, const Position> spot;
inline flecs::query<const ModelRef, const Position> renderables;
inline flecs::query<const ModelRef, const Position> shadow_casters;

inline void seed(flecs::world& ecs) {
    ambient = ecs.query_builder<const AmbientLight>()
        .cached()
        .build();
    directional = ecs.query_builder<const DirectionalLight>()
        .cached()
        .build();
    point = ecs.query_builder<const PointLight, const Position>()
        .cached()
        .build();
    spot = ecs.query_builder<const SpotLight, const Position>()
        .cached()
        .build();
    renderables = ecs.query_builder<const ModelRef, const Position>()
        .cached()
        .build();
    shadow_casters = ecs.query_builder<const ModelRef, const Position>()
        .with<ShadowCaster>()
        .cached()
        .build();
}

} // namespace queries

} // namespace graphics
