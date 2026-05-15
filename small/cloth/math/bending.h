#pragma once

#include "real.h"

namespace physics::bending {

struct Eval {
    Eigen::Vector3r n0, n1;          // triangle normals
    Eigen::Vector3r edge;            // shared edge v3-v2
    Real angle;                      // signed dihedral angle
    Real len;                        // edge length
    Real area;                       // diamond area: (A0 + A1) / 3
    Eigen::Vector3r d0, d1, d2, d3;  // gradient directions
};

// v0,v1 = opposite vertices, v2,v3 = shared edge
// iMSTK's looked promising so i took it (see imstkPbdDihedralConstraint.cpp)
inline bool eval(const Eigen::Vector3r& v0, const Eigen::Vector3r& v1,
                 const Eigen::Vector3r& v2, const Eigen::Vector3r& v3,
                 Eval& out) {
    out.edge = v3 - v2;
    out.len = out.edge.norm();
    if (out.len < 1e-10) return false;

    const Eigen::Vector3r e_hat = out.edge / out.len;

    // edge naming matters
    const Eigen::Vector3r e1 = v3 - v0;
    const Eigen::Vector3r e2 = v0 - v2;
    const Eigen::Vector3r e3 = v3 - v1;
    const Eigen::Vector3r e4 = v1 - v2;

    // triangle normals
    Eigen::Vector3r n0_raw = e1.cross(out.edge);
    Eigen::Vector3r n1_raw = out.edge.cross(e3);
    const Real a0 = n0_raw.norm();
    const Real a1 = n1_raw.norm();
    if (a0 < 1e-10 || a1 < 1e-10) return false;

    out.n0 = n0_raw / a0;
    out.n1 = n1_raw / a1;

    out.area = (a0 + a1) / 6;

    // signed dihedral angle, atan2(n1xn2.e, l*n1.n2)
    out.angle = std::atan2(
        out.n0.cross(out.n1).dot(out.edge),
        out.len * out.n0.dot(out.n1));

    // gradient, note the sign
    out.d0 = -(out.len / a0) * out.n0;
    out.d1 = -(out.len / a1) * out.n1;
    out.d2 = (out.edge.dot(e1) / (a0 * out.len)) * out.n0
           + (out.edge.dot(e3) / (a1 * out.len)) * out.n1;
    out.d3 = (out.edge.dot(e2) / (a0 * out.len)) * out.n0
           + (out.edge.dot(e4) / (a1 * out.len)) * out.n1;

    return true;
}

// FIXME: weight should use rest geometry for exact energy gradient. 

// E = 0.5 * k * (|e|^2 / A_diamond) * (theta - rest)^2
// from Discrete Shells
inline Real energy(Real stiffness, Real rest_angle, const Eval& e) {
    const Real diff = e.angle - rest_angle;
    const Real weight = e.len * e.len / e.area;
    return 0.5f * stiffness * weight * diff * diff;
}

inline void grad(Real stiffness, Real rest_angle, const Eval& e,
                 Eigen::Vector3r& g0, Eigen::Vector3r& g1,
                 Eigen::Vector3r& g2, Eigen::Vector3r& g3) {
    const Real diff = e.angle - rest_angle;
    const Real weight = e.len * e.len / e.area;
    const Real scale = stiffness * weight * diff;
    g0 = scale * e.d0;
    g1 = scale * e.d1;
    g2 = scale * e.d2;
    g3 = scale * e.d3;
}

// hess0: no bending stiffness at all (explicit)
inline void hess0(Real, Real, const Eval&,
                  Eigen::Matrix<Real, 12, 12>& H) {
    H.setZero();
}

// hess1: BW98-like approximation
// ArcSim (physics.cpp:234) do similar thing i guess
// PSD by construction but underestimates stiffness. (rank-1 after all)
inline void hess1(Real stiffness, Real rest_angle, const Eval& e,
                  Eigen::Matrix<Real, 12, 12>& H) {
    const Real weight = e.len * e.len / e.area;
    const Real scale = stiffness * weight;

    Eigen::Matrix<Real, 12, 1> d;
    d.segment<3>(0) = e.d0;
    d.segment<3>(3) = e.d1;
    d.segment<3>(6) = e.d2;
    d.segment<3>(9) = e.d3;

    H = scale * d * d.transpose();
}


// hess2: hess1 + diagonal shift
// *might* help cg to converge better for stiff bending.. eps = 1% of scale.
inline void hess2(Real stiffness, Real rest_angle, const Eval& e,
    Eigen::Matrix<Real, 12, 12>& H) {
    const Real weight = e.len * e.len / e.area;
    const Real scale = stiffness * weight;
    
    Eigen::Matrix<Real, 12, 1> d;
    d.segment<3>(0) = e.d0;
    d.segment<3>(3) = e.d1;
    d.segment<3>(6) = e.d2;
    d.segment<3>(9) = e.d3;
    
    const Real eps = scale * 0.01f;
    H = scale * d * d.transpose();
    for (int i = 0; i < 12; i++) H(i, i) += eps;
}

// TODO: full analytical Hess(theta) from Tamstorf-Grinspun 2013
// "Discrete bending forces and their Jacobians" Graphical Models 75.6.
// VegaFEM (clothBW.cpp:618) computes both terms analytically.

// default: hess1
inline void hess(Real stiffness, Real rest_angle, const Eval& e,
                 Eigen::Matrix<Real, 12, 12>& H) {
    hess1(stiffness, rest_angle, e, H);
}

// lineage (2003~2013) is well described here:
// https://wanghmin.github.io/publication/wang-2023-sdb/Wang-2023-SDB.pdf

} // namespace physics::bending
