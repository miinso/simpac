#pragma once

#include "types.h"

struct Position : engine::vec3<Position, Real> {
    using engine::vec3<Position, Real>::vec3;
    Position(const Vector3& v) : engine::vec3<Position, Real>((Real)v.x, (Real)v.y, (Real)v.z) {}
    operator Vector3() const { return Vector3{(float)x, (float)y, (float)z}; }
};
