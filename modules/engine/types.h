#pragma once

#include "real.h"

#include <Eigen/Dense>
#include <flecs.h>
#include <raylib.h>

#include <cstdint>
#include <cstddef>
#include <type_traits>

namespace engine {

template <typename Derived, typename Scalar = Real>
struct vec3 {
    using scalar_type = Scalar;

    Scalar x = Scalar(0);
    Scalar y = Scalar(0);
    Scalar z = Scalar(0);

    vec3() = default;
    vec3(Scalar x_, Scalar y_, Scalar z_) : x(x_), y(y_), z(z_) {}

    template <typename DerivedEigen>
    vec3(const Eigen::MatrixBase<DerivedEigen>& v)
        : x((Scalar)v.x()), y((Scalar)v.y()), z((Scalar)v.z()) {}

    Scalar* data() { return &x; }
    const Scalar* data() const { return &x; }

    Scalar& operator[](size_t i) { return data()[i]; }
    const Scalar& operator[](size_t i) const { return data()[i]; }

    Eigen::Map<Eigen::Matrix<Scalar, 3, 1, Eigen::DontAlign>> map() {
        return Eigen::Map<Eigen::Matrix<Scalar, 3, 1, Eigen::DontAlign>>(data());
    }
    Eigen::Map<const Eigen::Matrix<Scalar, 3, 1, Eigen::DontAlign>> map() const {
        return Eigen::Map<const Eigen::Matrix<Scalar, 3, 1, Eigen::DontAlign>>(data());
    }
};

template <typename Derived, typename Scalar = Real>
struct vec4 {
    using scalar_type = Scalar;

    Scalar x = Scalar(0);
    Scalar y = Scalar(0);
    Scalar z = Scalar(0);
    Scalar w = Scalar(0);

    vec4() = default;
    vec4(Scalar x_, Scalar y_, Scalar z_, Scalar w_) : x(x_), y(y_), z(z_), w(w_) {}

    template <typename DerivedEigen>
    vec4(const Eigen::MatrixBase<DerivedEigen>& v)
        : x((Scalar)v.x()), y((Scalar)v.y()), z((Scalar)v.z()), w((Scalar)v.w()) {}

    Scalar* data() { return &x; }
    const Scalar* data() const { return &x; }

    Scalar& operator[](size_t i) { return data()[i]; }
    const Scalar& operator[](size_t i) const { return data()[i]; }

    Eigen::Map<Eigen::Matrix<Scalar, 4, 1, Eigen::DontAlign>> map() {
        return Eigen::Map<Eigen::Matrix<Scalar, 4, 1, Eigen::DontAlign>>(data());
    }
    Eigen::Map<const Eigen::Matrix<Scalar, 4, 1, Eigen::DontAlign>> map() const {
        return Eigen::Map<const Eigen::Matrix<Scalar, 4, 1, Eigen::DontAlign>>(data());
    }
};

template <typename T>
inline void register_vec3_component(flecs::world& ecs, const char* name = nullptr) {
    if (name) {
        ecs.component<T>(name)
            .member("x", &T::x)
            .member("y", &T::y)
            .member("z", &T::z);
        return;
    }
    ecs.component<T>()
        .member("x", &T::x)
        .member("y", &T::y)
        .member("z", &T::z);
}

template <typename T>
inline void register_vec4_component(flecs::world& ecs, const char* name = nullptr) {
    if (name) {
        ecs.component<T>(name)
            .member("x", &T::x)
            .member("y", &T::y)
            .member("z", &T::z)
            .member("w", &T::w);
        return;
    }
    ecs.component<T>()
        .member("x", &T::x)
        .member("y", &T::y)
        .member("z", &T::z)
        .member("w", &T::w);
}

template <typename T>
inline void register_scalar_component(flecs::world& ecs, flecs::entity_t scalar_meta) {
    using Value = typename T::value_type;
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

} // namespace engine

struct vec3f : engine::vec3<vec3f, float> {
    using engine::vec3<vec3f, float>::vec3;
    vec3f(const Vector3& v) : engine::vec3<vec3f, float>(v.x, v.y, v.z) {}
    operator Vector3() const { return Vector3{x, y, z}; }
};

struct vec3d : engine::vec3<vec3d, double> {
    using engine::vec3<vec3d, double>::vec3;
};

struct vec3r : engine::vec3<vec3r, Real> {
    using engine::vec3<vec3r, Real>::vec3;
    vec3r(const Vector3& v)
        : engine::vec3<vec3r, Real>((Real)v.x, (Real)v.y, (Real)v.z) {}
    operator Vector3() const { return Vector3{(float)x, (float)y, (float)z}; }
};

struct vec4f : engine::vec4<vec4f, float> {
    using engine::vec4<vec4f, float>::vec4;
    vec4f(const Vector4& v) : engine::vec4<vec4f, float>(v.x, v.y, v.z, v.w) {}
    operator Vector4() const { return Vector4{x, y, z, w}; }
};

struct vec4d : engine::vec4<vec4d, double> {
    using engine::vec4<vec4d, double>::vec4;
};

struct vec4r : engine::vec4<vec4r, Real> {
    using engine::vec4<vec4r, Real>::vec4;
    vec4r(const Vector4& v)
        : engine::vec4<vec4r, Real>((Real)v.x, (Real)v.y, (Real)v.z, (Real)v.w) {}
    operator Vector4() const { return Vector4{(float)x, (float)y, (float)z, (float)w}; }
};

struct quatf : engine::vec4<quatf, float> {
    using engine::vec4<quatf, float>::vec4;
    quatf(const Vector4& v) : engine::vec4<quatf, float>(v.x, v.y, v.z, v.w) {}

    Eigen::Map<Eigen::Quaternion<float>, Eigen::DontAlign> map() {
        return Eigen::Map<Eigen::Quaternion<float>, Eigen::DontAlign>(data());
    }
    Eigen::Map<const Eigen::Quaternion<float>, Eigen::DontAlign> map() const {
        return Eigen::Map<const Eigen::Quaternion<float>, Eigen::DontAlign>(data());
    }
};

struct Position : engine::vec3<Position, Real> {
    using engine::vec3<Position, Real>::vec3;
    Position(const Vector3& v) : engine::vec3<Position, Real>((Real)v.x, (Real)v.y, (Real)v.z) {}
    operator Vector3() const { return Vector3{(float)x, (float)y, (float)z}; }
};
