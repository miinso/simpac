#pragma once

#include <Eigen/Dense>

// matches graphics::scalar_real
using Real = float;

namespace Eigen {
using Vector3r = Matrix<Real, 3, 1, DontAlign>;
using Vector4r = Matrix<Real, 4, 1, DontAlign>;
using Matrix2r = Matrix<Real, 2, 2>;
using Matrix3r = Matrix<Real, 3, 3>;
using VectorXr = Matrix<Real, Dynamic, 1>;
using ArrayXr = Array<Real, Dynamic, 1>;
using MatrixXr = Matrix<Real, Dynamic, Dynamic>;
} // namespace Eigen
