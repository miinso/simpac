#pragma once

#include <raylib.h>
#include "raygizmo.h"

// Light types
enum LightType { LIGHT_DIRECTIONAL = 0, LIGHT_POINT, LIGHT_SPOT };

// Pure data for light
struct Light {
    int type;
    bool enabled;
    Vector3 position;
    Vector3 target;
    Color color;
    float intensity;
};

// Shader locations for a light (rendering concern)
struct LightShaderLocs {
    int typeLoc;
    int enabledLoc;
    int positionLoc;
    int targetLoc;
    int colorLoc;
    int intensityLoc;
};

// Gizmo transform for interactive manipulation
struct GizmoTransform {
    Transform value;
};

// Scene state
struct Scene {
    float elapsed;
};
