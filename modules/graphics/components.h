#pragma once

#include <flecs.h>
#include "camera.h"
#include "tags.h"

namespace graphics {

struct ColorRGBA {
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
    float a = 1.0f;

    static inline void meta(flecs::world& ecs) {
        ecs.component<ColorRGBA>()
            .member("r", &ColorRGBA::r)
            .member("g", &ColorRGBA::g)
            .member("b", &ColorRGBA::b)
            .member("a", &ColorRGBA::a);
    }
};

[[nodiscard]] inline ColorRGBA to_rgba(Color color) {
    return {
        color.r / 255.0f,
        color.g / 255.0f,
        color.b / 255.0f,
        color.a / 255.0f
    };
}

[[nodiscard]] inline Color to_color(const ColorRGBA& color) {
    auto to_byte = [](float v) -> unsigned char {
        float clamped = v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
        return (unsigned char)(clamped * 255.0f);
    };
    return {to_byte(color.r), to_byte(color.g), to_byte(color.b), to_byte(color.a)};
}

namespace components {

inline void register_components(flecs::world& world) {
    register_camera_components(world);
    ColorRGBA::meta(world);
    world.component<Configurable>();
}

} // namespace components
} // namespace graphics
