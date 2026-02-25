#pragma once

#include <raylib.h>
#include "raygizmo.h"

// Gizmo transform for interactive manipulation
struct GizmoTransform {
    Transform value;
};

// Scene state
struct Scene {
    float elapsed;
};
