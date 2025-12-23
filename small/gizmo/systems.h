#pragma once

#include "components.h"
#include <raylib.h>
#include "raygizmo.h"

namespace systems {

// Initialize light shader locations for a specific light index
inline void init_light_shader_locs(LightShaderLocs& locs, Shader shader, int index) {
    locs.enabledLoc = GetShaderLocation(shader, TextFormat("lights[%i].enabled", index));
    locs.typeLoc = GetShaderLocation(shader, TextFormat("lights[%i].type", index));
    locs.positionLoc = GetShaderLocation(shader, TextFormat("lights[%i].position", index));
    locs.targetLoc = GetShaderLocation(shader, TextFormat("lights[%i].target", index));
    locs.colorLoc = GetShaderLocation(shader, TextFormat("lights[%i].color", index));
    locs.intensityLoc = GetShaderLocation(shader, TextFormat("lights[%i].intensity", index));
}

// Update light shader uniforms
inline void update_light_shader(Shader shader, const Light& light, const LightShaderLocs& locs) {
    int enabled = light.enabled ? 1 : 0;
    SetShaderValue(shader, locs.enabledLoc, &enabled, SHADER_UNIFORM_INT);
    SetShaderValue(shader, locs.typeLoc, &light.type, SHADER_UNIFORM_INT);

    float position[3] = {light.position.x, light.position.y, light.position.z};
    SetShaderValue(shader, locs.positionLoc, position, SHADER_UNIFORM_VEC3);

    float target[3] = {light.target.x, light.target.y, light.target.z};
    SetShaderValue(shader, locs.targetLoc, target, SHADER_UNIFORM_VEC3);

    float color[4] = {light.color.r / 255.0f, light.color.g / 255.0f,
                      light.color.b / 255.0f, light.color.a / 255.0f};
    SetShaderValue(shader, locs.colorLoc, color, SHADER_UNIFORM_VEC4);
    SetShaderValue(shader, locs.intensityLoc, &light.intensity, SHADER_UNIFORM_FLOAT);
}

// Sync light position from gizmo transform
inline void sync_light_from_gizmo(Light& light, const GizmoTransform& gt) {
    light.position = gt.value.translation;
}

// Draw light visualization
inline void draw_light(const Light& light) {
    if (light.enabled) {
        DrawSphereEx(light.position, 0.2f, 8, 8, light.color);
    } else {
        DrawSphereWires(light.position, 0.2f, 8, 8, ColorAlpha(light.color, 0.3f));
    }
}

// Draw and update gizmo, returns true if transform changed
inline void draw_light_gizmo(GizmoTransform& gt) {
    DrawGizmo3D(GIZMO_TRANSLATE, &gt.value);
}

} // namespace systems
