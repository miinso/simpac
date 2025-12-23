// core/types.hpp
#pragma once
#include <Common.h>
#include <Eigen/Dense>

// undef raylib const
#ifdef PI
#    undef PI
#endif

#ifdef EPSILON
#    undef EPSILON
#endif

// types from pbd/Common.h
// using Vector2r = Eigen::Matrix<Real, 2, 1, Eigen::DontAlign>;
// using Vector3r = Eigen::Matrix<Real, 3, 1, Eigen::DontAlign>;
// using Vector4r = Eigen::Matrix<Real, 4, 1, Eigen::DontAlign>;
// using Vector5r = Eigen::Matrix<Real, 5, 1, Eigen::DontAlign>;
// using Vector6r = Eigen::Matrix<Real, 6, 1, Eigen::DontAlign>;
// using Matrix2r = Eigen::Matrix<Real, 2, 2, Eigen::DontAlign>;
// using Matrix3r = Eigen::Matrix<Real, 3, 3, Eigen::DontAlign>;
// using Matrix4r = Eigen::Matrix<Real, 4, 4, Eigen::DontAlign>;
// using Vector2i = Eigen::Matrix<int, 2, 1, Eigen::DontAlign>;
// using AlignedBox2r = Eigen::AlignedBox<Real, 2>;
// using AlignedBox3r = Eigen::AlignedBox<Real, 3>;
// using AngleAxisr = Eigen::AngleAxis<Real>;
// using Quaternionr = Eigen::Quaternion<Real, Eigen::DontAlign>;

// custom
using Vector3i = Eigen::Matrix<int, 3, 1, Eigen::DontAlign>;

namespace phys {

    // constants
    namespace constants {
        constexpr Real PI = 3.14159265359f;
        constexpr Real EPSILON = 1e-6f;
        const Vector3r GRAVITY = {0.0f, -9.81f, 0.0f};
    } // namespace constants
} // namespace phys