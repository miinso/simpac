#pragma once

#include <Eigen/Dense>
#include <flecs.h>
#include <raylib.h>

#include <cstdint>
#include <cstddef>
#include <type_traits>

namespace graphics {

// default scalar for real vector aliases
using scalar_real = float;

// crtp base for 3d vectors with plain x y z storage
template <typename Derived, typename Scalar = scalar_real>
struct vec3 {
    using scalar_type = Scalar;

    Scalar x = Scalar(0);
    Scalar y = Scalar(0);
    Scalar z = Scalar(0);

    vec3() = default;
    vec3(Scalar x_, Scalar y_, Scalar z_) : x(x_), y(y_), z(z_) {}

    // construct from eigen vectors and matrices
    template <typename DerivedEigen>
    vec3(const Eigen::MatrixBase<DerivedEigen>& v)
        : x((Scalar)v.x()), y((Scalar)v.y()), z((Scalar)v.z()) {}

    Scalar* data() { return &x; }
    const Scalar* data() const { return &x; }

    Scalar& operator[](size_t i) { return data()[i]; }
    const Scalar& operator[](size_t i) const { return data()[i]; }

    // non owning eigen view for math ops without copies
    Eigen::Map<Eigen::Matrix<Scalar, 3, 1, Eigen::DontAlign>> map() {
        return Eigen::Map<Eigen::Matrix<Scalar, 3, 1, Eigen::DontAlign>>(data());
    }
    Eigen::Map<const Eigen::Matrix<Scalar, 3, 1, Eigen::DontAlign>> map() const {
        return Eigen::Map<const Eigen::Matrix<Scalar, 3, 1, Eigen::DontAlign>>(data());
    }
};

// crtp base for 4d vectors with plain x y z w storage
template <typename Derived, typename Scalar = scalar_real>
struct vec4 {
    using scalar_type = Scalar;

    Scalar x = Scalar(0);
    Scalar y = Scalar(0);
    Scalar z = Scalar(0);
    Scalar w = Scalar(0);

    vec4() = default;
    vec4(Scalar x_, Scalar y_, Scalar z_, Scalar w_) : x(x_), y(y_), z(z_), w(w_) {}

    // construct from eigen vectors and matrices
    template <typename DerivedEigen>
    vec4(const Eigen::MatrixBase<DerivedEigen>& v)
        : x((Scalar)v.x()), y((Scalar)v.y()), z((Scalar)v.z()), w((Scalar)v.w()) {}

    Scalar* data() { return &x; }
    const Scalar* data() const { return &x; }

    Scalar& operator[](size_t i) { return data()[i]; }
    const Scalar& operator[](size_t i) const { return data()[i]; }

    // non owning eigen view for math ops without copies
    Eigen::Map<Eigen::Matrix<Scalar, 4, 1, Eigen::DontAlign>> map() {
        return Eigen::Map<Eigen::Matrix<Scalar, 4, 1, Eigen::DontAlign>>(data());
    }
    Eigen::Map<const Eigen::Matrix<Scalar, 4, 1, Eigen::DontAlign>> map() const {
        return Eigen::Map<const Eigen::Matrix<Scalar, 4, 1, Eigen::DontAlign>>(data());
    }
    // NOTE: we offload vector ops to eigens'
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

// f32 vector with raylib vector3 interop
struct vec3f : vec3<vec3f, float> {
    using vec3<vec3f, float>::vec3;
    vec3f(const Vector3& v)
        : vec3<vec3f, float>((float)v.x, (float)v.y, (float)v.z) {}
    operator Vector3() const { return Vector3{x, y, z}; }
};

struct vec3d : vec3<vec3d, double> {
    using vec3<vec3d, double>::vec3;
};

// real vector bound to scalar_real project default precision
struct vec3r : vec3<vec3r, scalar_real> {
    using vec3<vec3r, scalar_real>::vec3;
    vec3r(const Vector3& v)
        : vec3<vec3r, scalar_real>((scalar_real)v.x, (scalar_real)v.y, (scalar_real)v.z) {}
    operator Vector3() const { return Vector3{(float)x, (float)y, (float)z}; }
};

// f32 vector with raylib vector4 interop
struct vec4f : vec4<vec4f, float> {
    using vec4<vec4f, float>::vec4;
    vec4f(const Vector4& v)
        : vec4<vec4f, float>((float)v.x, (float)v.y, (float)v.z, (float)v.w) {}
    operator Vector4() const { return Vector4{x, y, z, w}; }
};

struct vec4d : vec4<vec4d, double> {
    using vec4<vec4d, double>::vec4;
};

// real 4d vector bound to scalar_real
struct vec4r : vec4<vec4r, scalar_real> {
    using vec4<vec4r, scalar_real>::vec4;
    vec4r(const Vector4& v)
        : vec4<vec4r, scalar_real>((scalar_real)v.x, (scalar_real)v.y, (scalar_real)v.z, (scalar_real)v.w) {}
    operator Vector4() const { return Vector4{(float)x, (float)y, (float)z, (float)w}; }
};

} // namespace graphics
