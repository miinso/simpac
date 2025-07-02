#pragma once

#include <raylib.h>

namespace rendering {
    struct Circle {
        float radius;
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

    struct Rotation {
        float angle; // degree
    };

    struct ProgressBar {
        float min_val;
        float max_val;
        float current_val;
        Rectangle rectangle;
    };
} // namespace rendering
