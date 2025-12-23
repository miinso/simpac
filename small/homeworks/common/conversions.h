#pragma once

#include "Eigen/Dense"
#include <raylib.h>

// Eigen -> raylib
inline Vector2 toRay(const Eigen::Vector2f& v) { return {v.x(), v.y()}; }
inline Vector3 toRay(const Eigen::Vector3f& v) { return {v.x(), v.y(), v.z()}; }
inline Vector4 toRay(const Eigen::Vector4f& v) { return {v.x(), v.y(), v.z(), v.w()}; }
inline Matrix toRay(const Eigen::Matrix4f& m) {
    Matrix result;
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            (&result.m0)[i * 4 + j] = m(j, i);
    return result;
}

// raylib -> Eigen
inline Eigen::Vector2f toEig(const Vector2& v) { return {v.x, v.y}; }
inline Eigen::Vector3f toEig(const Vector3& v) { return {v.x, v.y, v.z}; }
inline Eigen::Vector4f toEig(const Vector4& v) { return {v.x, v.y, v.z, v.w}; }
inline Eigen::Matrix4f toEig(const Matrix& m) {
    Eigen::Matrix4f result;
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            result(j, i) = (&m.m0)[i * 4 + j];
    return result;
}
