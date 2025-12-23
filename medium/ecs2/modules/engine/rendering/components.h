#pragma once

#include <flecs.h>
#include <raylib.h>

namespace rendering {
    struct Circle {
        float radius;
    };

    struct Box {
        float width;
        float height;
    };

    // combined with an integer as a pair <Priority, int32_t>, consumed by `group_by()`
    struct Priority {
        int priority;
    };

    struct Visible {};

    struct Renderable {
        Texture2D texture;
        Vector2 draw_offset;
        float scale;
        Color tint;
    };

    struct TrackingCamera {
        flecs::entity target;
        Camera2D camera{0};
    };
} // namespace rendering
