#pragma once

#include <Eigen/Dense>
#include <cstddef>

using Real = float;
namespace Eigen {
using Vector3r = Matrix<Real, 3, 1, DontAlign>;
}

/*
  crtp keeps vec3 components uniform and lightweight.
  plain fields for flecs; map() gives an eigen view when needed.
*/
template <typename Derived>
struct Vec3Component {
    Real x = Real(0);
    Real y = Real(0);
    Real z = Real(0);

    Vec3Component() = default;
    Vec3Component(Real x_, Real y_, Real z_) : x(x_), y(y_), z(z_) {}

    template <typename DerivedEigen>
    Vec3Component(const Eigen::MatrixBase<DerivedEigen>& v)
        : x(Real(v.x())), y(Real(v.y())), z(Real(v.z())) {}

    Real* data() { return &x; }
    const Real* data() const { return &x; }

    Real& operator[](size_t i) { return data()[i]; }
    const Real& operator[](size_t i) const { return data()[i]; }

    Eigen::Map<Eigen::Vector3r> map() { return Eigen::Map<Eigen::Vector3r>(data()); }
    Eigen::Map<const Eigen::Vector3r> map() const { return Eigen::Map<const Eigen::Vector3r>(data()); }
};

struct Position : Vec3Component<Position> {
    using Vec3Component<Position>::Vec3Component;
};

struct Velocity : Vec3Component<Velocity> {
    using Vec3Component<Velocity>::Vec3Component;
};
