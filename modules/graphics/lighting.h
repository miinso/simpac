#pragma once

#include <raylib.h>

namespace graphics {

// Maximum lights supported by shader (matches rlights.h)
inline constexpr int MAX_LIGHTS = 4;

// Light types
enum LightType {
    LIGHT_DIRECTIONAL = 0,
    LIGHT_POINT = 1
};

// Light data structure
struct Light {
    int type = LIGHT_POINT;
    bool enabled = true;
    Vector3 position = {0.0f, 0.0f, 0.0f};
    Vector3 target = {0.0f, 0.0f, 0.0f};
    Color color = WHITE;
    float attenuation = 1.0f;

    // Shader uniform locations (set by create_light)
    int enabled_loc = -1;
    int type_loc = -1;
    int position_loc = -1;
    int target_loc = -1;
    int color_loc = -1;
    int attenuation_loc = -1;
};

// Internal state
namespace detail {
    inline Shader lighting_shader{};
    inline int lights_count = 0;
    inline bool lighting_enabled = false;
}

// Check if lighting is currently enabled
[[nodiscard]] inline bool is_lighting_enabled() {
    return detail::lighting_enabled;
}

// Get the lighting shader (for advanced use)
[[nodiscard]] inline Shader& get_lighting_shader() {
    return detail::lighting_shader;
}

// Enable lighting with custom shaders
// vs_path: vertex shader path (e.g., "resources/shaders/glsl330/lighting.vs")
// fs_path: fragment shader path (e.g., "resources/shaders/glsl330/lighting.fs")
inline void enable_lighting(const char* vs_path, const char* fs_path) {
    detail::lighting_shader = LoadShader(vs_path, fs_path);
    detail::lighting_shader.locs[SHADER_LOC_VECTOR_VIEW] =
        GetShaderLocation(detail::lighting_shader, "viewPos");
    detail::lighting_enabled = true;
    detail::lights_count = 0;
}

// Set ambient light level
inline void set_ambient_light(float r, float g, float b, float a = 1.0f) {
    if (!detail::lighting_enabled) return;

    int ambient_loc = GetShaderLocation(detail::lighting_shader, "ambient");
    float ambient[4] = {r, g, b, a};
    SetShaderValue(detail::lighting_shader, ambient_loc, ambient, SHADER_UNIFORM_VEC4);
}

// Create a new light and register it with the shader
// Returns the created light (with shader locations set)
[[nodiscard]] inline Light create_light(LightType type, Vector3 position, Vector3 target, Color color) {
    Light light{};

    if (!detail::lighting_enabled || detail::lights_count >= MAX_LIGHTS) {
        return light;
    }

    light.enabled = true;
    light.type = type;
    light.position = position;
    light.target = target;
    light.color = color;

    // Get shader locations for this light index
    int idx = detail::lights_count;
    Shader& shader = detail::lighting_shader;

    light.enabled_loc = GetShaderLocation(shader, TextFormat("lights[%i].enabled", idx));
    light.type_loc = GetShaderLocation(shader, TextFormat("lights[%i].type", idx));
    light.position_loc = GetShaderLocation(shader, TextFormat("lights[%i].position", idx));
    light.target_loc = GetShaderLocation(shader, TextFormat("lights[%i].target", idx));
    light.color_loc = GetShaderLocation(shader, TextFormat("lights[%i].color", idx));

    detail::lights_count++;

    return light;
}

// Update light values in shader (call after modifying light properties)
inline void update_light(const Light& light) {
    if (!detail::lighting_enabled) return;

    Shader& shader = detail::lighting_shader;

    SetShaderValue(shader, light.enabled_loc, &light.enabled, SHADER_UNIFORM_INT);
    SetShaderValue(shader, light.type_loc, &light.type, SHADER_UNIFORM_INT);

    float position[3] = {light.position.x, light.position.y, light.position.z};
    SetShaderValue(shader, light.position_loc, position, SHADER_UNIFORM_VEC3);

    float target[3] = {light.target.x, light.target.y, light.target.z};
    SetShaderValue(shader, light.target_loc, target, SHADER_UNIFORM_VEC3);

    float color[4] = {
        static_cast<float>(light.color.r) / 255.0f,
        static_cast<float>(light.color.g) / 255.0f,
        static_cast<float>(light.color.b) / 255.0f,
        static_cast<float>(light.color.a) / 255.0f
    };
    SetShaderValue(shader, light.color_loc, color, SHADER_UNIFORM_VEC4);
}

// Update shader with camera position (call each frame when lighting is enabled)
inline void update_lighting_camera_pos(const Vector3& camera_pos) {
    if (!detail::lighting_enabled) return;

    float pos[3] = {camera_pos.x, camera_pos.y, camera_pos.z};
    SetShaderValue(detail::lighting_shader,
                   detail::lighting_shader.locs[SHADER_LOC_VECTOR_VIEW],
                   pos, SHADER_UNIFORM_VEC3);
}

// Cleanup lighting resources
inline void disable_lighting() {
    if (detail::lighting_enabled) {
        UnloadShader(detail::lighting_shader);
        detail::lighting_enabled = false;
        detail::lights_count = 0;
    }
}

} // namespace graphics
