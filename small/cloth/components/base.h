#pragma once

#include <Eigen/Dense>
#include <Eigen/Sparse>
#include <flecs.h>

#include <cstddef>
#include <type_traits>

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

template <typename Derived>
struct vec3 {
    Real x = Real(0);
    Real y = Real(0);
    Real z = Real(0);

    vec3() = default;
    vec3(Real x_, Real y_, Real z_) : x(x_), y(y_), z(z_) {}

    template <typename DerivedEigen>
    vec3(const Eigen::MatrixBase<DerivedEigen>& v)
        : x(Real(v.x())), y(Real(v.y())), z(Real(v.z())) {}

    Real* data() { return &x; }
    const Real* data() const { return &x; }

    Real& operator[](size_t i) { return data()[i]; }
    const Real& operator[](size_t i) const { return data()[i]; }

    Eigen::Map<Eigen::Vector3r> map() { return Eigen::Map<Eigen::Vector3r>(data()); }
    Eigen::Map<const Eigen::Vector3r> map() const { return Eigen::Map<const Eigen::Vector3r>(data()); }
};

template <typename Derived>
struct vec4 {
    Real x = Real(0);
    Real y = Real(0);
    Real z = Real(0);
    Real w = Real(0);

    vec4() = default;
    vec4(Real x_, Real y_, Real z_, Real w_) : x(x_), y(y_), z(z_), w(w_) {}

    template <typename DerivedEigen>
    vec4(const Eigen::MatrixBase<DerivedEigen>& v)
        : x(Real(v.x())), y(Real(v.y())), z(Real(v.z())), w(Real(v.w())) {}

    Real* data() { return &x; }
    const Real* data() const { return &x; }

    Real& operator[](size_t i) { return data()[i]; }
    const Real& operator[](size_t i) const { return data()[i]; }

    Eigen::Map<Eigen::Vector4r> map() { return Eigen::Map<Eigen::Vector4r>(data()); }
    Eigen::Map<const Eigen::Vector4r> map() const { return Eigen::Map<const Eigen::Vector4r>(data()); }
};

template <typename Derived, typename Value>
struct scalar {
    Value value = Value(0);

    scalar() = default;
    scalar(Value v) : value(v) {}

    scalar& operator=(Value v) {
        value = v;
        return *this;
    }

    operator Value&() { return value; }
    operator const Value&() const { return value; }
};

template <typename T>
inline void register_vec3(flecs::world& ecs) {
    ecs.component<T>()
        .member("x", &T::x)
        .member("y", &T::y)
        .member("z", &T::z);
}

template <typename T>
inline void register_vec4(flecs::world& ecs) {
    ecs.component<T>()
        .member("x", &T::x)
        .member("y", &T::y)
        .member("z", &T::z)
        .member("w", &T::w);
}

template <typename T>
inline void register_scalar(flecs::world& ecs, flecs::entity_t scalar_meta) {
    using Value = std::remove_cv_t<std::remove_reference_t<decltype(T::value)>>;
    auto opaque = ecs.component<T>().opaque(scalar_meta);
    if constexpr (std::is_integral_v<Value>) {
        opaque.serialize([](const flecs::serializer* s, const T* data) {
                if (!data) return 0;
                s->value(static_cast<int64_t>(data->value));
                return 0;
            })
            .assign_int([](T* dst, int64_t value) {
                if (!dst) return;
                dst->value = static_cast<Value>(value);
            });
    } else {
        opaque.serialize([](const flecs::serializer* s, const T* data) {
                if (!data) return 0;
                s->value(static_cast<double>(data->value));
                return 0;
            })
            .assign_float([](T* dst, double value) {
                if (!dst) return;
                dst->value = static_cast<Value>(value);
            });
    }
}
