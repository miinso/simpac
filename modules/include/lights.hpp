#pragma once
#include "graphics.h"

namespace graphics {

    // light type enum
    enum LightType { LIGHT_DIRECTIONAL = 0, LIGHT_POINT = 1 };

    // light data structure
    struct Light {
        bool enabled = true;
        int type = LIGHT_POINT;
        rl::Vector3 position;
        rl::Vector3 target;
        rl::Color color;
        float attenuation = 0.0f;

        // shader locations
        int enabled_loc = -1;
        int type_loc = -1;
        int position_loc = -1;
        int target_loc = -1;
        int color_loc = -1;
        int attenuation_loc = -1;
    };

    // create new light and get shader locations
    inline Light create_light(
        int type, rl::Vector3 position, rl::Vector3 target, rl::Color color, rl::Shader shader, int light_index) {
        Light light; // use regular initialization

        // initialize basic properties
        light.enabled = true;
        light.type = type;
        light.position = position;
        light.target = target;
        light.color = color;

        // get shader locations using sprintf
        char buf[64];

        snprintf(buf, sizeof(buf), "lights[%d].enabled", light_index);
        light.enabled_loc = rl::GetShaderLocation(shader, buf);

        snprintf(buf, sizeof(buf), "lights[%d].type", light_index);
        light.type_loc = rl::GetShaderLocation(shader, buf);

        snprintf(buf, sizeof(buf), "lights[%d].position", light_index);
        light.position_loc = rl::GetShaderLocation(shader, buf);

        snprintf(buf, sizeof(buf), "lights[%d].target", light_index);
        light.target_loc = rl::GetShaderLocation(shader, buf);

        snprintf(buf, sizeof(buf), "lights[%d].color", light_index);
        light.color_loc = rl::GetShaderLocation(shader, buf);

        return light;
    }

    // update light properties in shader
    inline void update_light(rl::Shader shader, const Light& light) {
        rl::SetShaderValue(shader, light.enabled_loc, &light.enabled, rl::SHADER_UNIFORM_INT);
        rl::SetShaderValue(shader, light.type_loc, &light.type, rl::SHADER_UNIFORM_INT);
        rl::SetShaderValue(shader, light.position_loc, &light.position, rl::SHADER_UNIFORM_VEC3);
        rl::SetShaderValue(shader, light.target_loc, &light.target, rl::SHADER_UNIFORM_VEC3);

        // normalize color to 0-1 range
        float color_normalized[4] = {static_cast<float>(light.color.r) / 255.0f,
                                     static_cast<float>(light.color.g) / 255.0f,
                                     static_cast<float>(light.color.b) / 255.0f,
                                     static_cast<float>(light.color.a) / 255.0f};
        rl::SetShaderValue(shader, light.color_loc, color_normalized, rl::SHADER_UNIFORM_VEC4);
    }

} // namespace graphics