#pragma once

#include "real.h"
#include <Eigen/SVD>

namespace physics::corot {

struct Eval {
    Eigen::Matrix<Real, 3, 2> F;
    Eigen::Matrix<Real, 3, 2> R;
    Real tr;
};

inline bool eval(const Eigen::Vector3r& x0,
                 const Eigen::Vector3r& x1,
                 const Eigen::Vector3r& x2,
                 const Eigen::Matrix2r& dm_inv,
                 Eval& out) {
    Eigen::Matrix<Real, 3, 2> Ds;
    Ds.col(0) = x1 - x0;
    Ds.col(1) = x2 - x0;

    out.F = Ds * dm_inv;

    Eigen::JacobiSVD<Eigen::Matrix<Real, 3, 2>> svd(out.F, Eigen::ComputeFullU | Eigen::ComputeFullV);
    Eigen::Matrix<Real, 3, 3> U = svd.matrixU();
    Eigen::Matrix2r V = svd.matrixV();
    out.R = U.leftCols<2>() * V.transpose();
    out.tr = (out.R.transpose() * out.F).trace();
    return true;
}

inline Real energy(const Eval& eval,
                   Real mu,
                   Real lambda,
                   Real area,
                   Real thickness = Real(1)) {
    Real diff = eval.tr - Real(2);
    Real psi = mu * (eval.F - eval.R).squaredNorm() + Real(0.5) * lambda * diff * diff;
    return area * thickness * psi;
}

inline void grad(const Eval& eval,
                 const Eigen::Matrix2r& dm_inv,
                 Real mu,
                 Real lambda,
                 Real area,
                 Eigen::Vector3r& g0,
                 Eigen::Vector3r& g1,
                 Eigen::Vector3r& g2,
                 Real thickness = Real(1)) {
    Real diff = eval.tr - Real(2);
    Eigen::Matrix<Real, 3, 2> P =
        (Real(2) * mu) * (eval.F - eval.R) + lambda * diff * eval.R;
    Eigen::Matrix<Real, 3, 2> H = (area * thickness) * P * dm_inv.transpose();

    g1 = H.col(0);
    g2 = H.col(1);
    g0 = -g1 - g2;
}

inline void hess(const Eval& eval,
                 const Eigen::Matrix2r& dm_inv,
                 Real mu,
                 Real lambda,
                 Real area,
                 Eigen::Matrix<Real, 9, 9>& K,
                 Real thickness = Real(1)) {
    Eigen::Matrix<Real, 6, 6> Kf = Eigen::Matrix<Real, 6, 6>::Identity() * (Real(2) * mu);
    Eigen::Matrix<Real, 6, 1> r;
    r.segment<3>(0) = eval.R.col(0);
    r.segment<3>(3) = eval.R.col(1);
    Kf += lambda * (r * r.transpose());

    Real a = dm_inv(0, 0);
    Real b = dm_inv(0, 1);
    Real c = dm_inv(1, 0);
    Real d = dm_inv(1, 1);

    Eigen::Matrix3r I = Eigen::Matrix3r::Identity();
    Eigen::Matrix<Real, 6, 3> B1;
    Eigen::Matrix<Real, 6, 3> B2;

    B1.block<3, 3>(0, 0) = a * I;
    B1.block<3, 3>(3, 0) = b * I;
    B2.block<3, 3>(0, 0) = c * I;
    B2.block<3, 3>(3, 0) = d * I;

    Eigen::Matrix<Real, 6, 3> B0 = -B1 - B2;
    Eigen::Matrix<Real, 6, 9> B;
    B.block<6, 3>(0, 0) = B0;
    B.block<6, 3>(0, 3) = B1;
    B.block<6, 3>(0, 6) = B2;

    K = (area * thickness) * (B.transpose() * Kf * B);

    // TODO: double check with kugelstadt2018's
}

} // namespace physics::corot
