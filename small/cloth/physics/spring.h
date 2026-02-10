#pragma once

#include "../components.h"

namespace physics::spring {

constexpr double kEpsilon = 1e-6;

struct Eval {
    Eigen::Vector3r dir;
    Real length;
    Real strain;
    Real rel_vel;
};

struct Sample {
    flecs::entity a;
    flecs::entity b;
    bool a_pinned = false;
    bool b_pinned = false;
    int idx_a = 0;
    int idx_b = 0;
    Eigen::Vector3r x_a = Eigen::Vector3r::Zero();
    Eigen::Vector3r x_b = Eigen::Vector3r::Zero();
    Eigen::Vector3r v_a = Eigen::Vector3r::Zero();
    Eigen::Vector3r v_b = Eigen::Vector3r::Zero();
    Eval eval = {};
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

inline bool sample(const Spring& spring, Sample& out, bool with_index = false) {
    out.a = spring.e1;
    out.b = spring.e2;

    out.a_pinned = out.a.has<IsPinned>();
    out.b_pinned = out.b.has<IsPinned>();

    out.x_a = out.a.get<Position>().map();
    out.x_b = out.b.get<Position>().map();
    out.v_a = out.a.get<Velocity>().map();
    out.v_b = out.b.get<Velocity>().map();

    if (with_index) {
        out.idx_a = out.a.get<ParticleIndex>();
        out.idx_b = out.b.get<ParticleIndex>();
    }

    return eval(out.x_a, out.x_b, out.v_a, out.v_b, spring.rest_length, out.eval);
}

inline bool force(const Spring& spring,
                  Real k_s,
                  Real k_d,
                  Sample& out,
                  Eigen::Vector3r& f_a,
                  Eigen::Vector3r& f_b) {
    if (!sample(spring, out, false)) return false;
    auto g_a = grad(k_s, k_d, out.eval);
    f_a = -g_a;
    f_b = -f_a;
    return true;
}

} // namespace physics::spring
