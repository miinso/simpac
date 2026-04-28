#pragma once

#include "real.h"

namespace physics::spring {

constexpr double kEpsilon = 1e-6;

struct Eval {
    Eigen::Vector3r dir;
    Real length;
    Real strain;
    Real rel_vel;
};

inline bool eval(const Eigen::Vector3r& x_a,
                 const Eigen::Vector3r& x_b,
                 const Eigen::Vector3r& v_a,
                 const Eigen::Vector3r& v_b,
                 Real rest_length,
                 Eval& out) {
    auto diff = x_a - x_b;
    out.length = diff.norm();
    if (out.length < kEpsilon) return false;
    out.dir = diff / out.length;

    out.strain = (out.length - rest_length) / rest_length;

    auto diff_v = v_a - v_b;
    out.rel_vel = (diff_v / rest_length).dot(out.dir);
    return true;
}

inline Eigen::Vector3r grad(Real k_s, Real k_d, const Eval& eval) {
    return (k_s * eval.strain + k_d * eval.rel_vel) * eval.dir;
}

inline Real energy(Real k_s, const Eval& eval) {
    return Real(0.5) * k_s * eval.strain * eval.strain;
}

inline Eigen::Matrix3r hess(Real k_s, Real rest_length, const Eval& eval) {
    Real ratio = rest_length / eval.length;
    Eigen::Matrix3r I = Eigen::Matrix3r::Identity();
    Eigen::Matrix3r ddT = eval.dir * eval.dir.transpose();
    return k_s * ((1.0 - ratio) * I + ratio * ddT);
}

} // namespace physics::spring
