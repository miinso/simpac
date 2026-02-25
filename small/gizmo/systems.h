#pragma once

#include "components.h"
#include "graphics.h"
#include <raylib.h>
#include "raygizmo.h"

namespace systems {

// --- directional light helpers ---

inline void draw_directional(const graphics::DirectionalLight& dl) {
    // draw a small arrow in the direction
    Color c = graphics::to_color(dl.color);
    if (dl.enabled) {
        DrawSphere({0, 5, 0}, 0.2f, c);
        DrawLine3D({0, 5, 0}, {dl.direction.x * 2, 5 + dl.direction.y * 2, dl.direction.z * 2}, c);
    } else {
        DrawSphereWires({0, 5, 0}, 0.2f, 8, 8, ColorAlpha(c, 0.3f));
    }
}

// --- point light helpers ---

inline void draw_point(const graphics::PointLight& pl, const graphics::Position& pos) {
    Color c = graphics::to_color(pl.color);
    Vector3 p = {pos.x, pos.y, pos.z};
    if (pl.enabled) {
        DrawSphereEx(p, 0.2f, 8, 8, c);
    } else {
        DrawSphereWires(p, 0.2f, 8, 8, ColorAlpha(c, 0.3f));
    }
}

// --- spot light helpers ---

inline void draw_spot(const graphics::SpotLight& sl, const graphics::Position& pos) {
    Color c = graphics::to_color(sl.color);
    Vector3 p = {pos.x, pos.y, pos.z};
    if (sl.enabled) {
        DrawSphereEx(p, 0.2f, 8, 8, c);
        // draw direction line
        Vector3 end = {p.x + sl.direction.x, p.y + sl.direction.y, p.z + sl.direction.z};
        DrawLine3D(p, end, c);
    } else {
        DrawSphereWires(p, 0.2f, 8, 8, ColorAlpha(c, 0.3f));
    }
}

} // namespace systems
