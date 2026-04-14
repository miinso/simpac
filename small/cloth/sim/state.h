#pragma once

#include "real.h"

namespace physics {

// dynamic per-step data, created from Model
// solver-specific working buffers (lambdas, deltas, jacobi_dx ..) are owned by the solver, not here
struct State {
    Eigen::VectorXr particle_q;          // positions [n * 3]
    Eigen::VectorXr particle_qd;         // velocities [n * 3]
    Eigen::VectorXr particle_f;          // forces [n * 3]

    int particle_count = 0;

    // per-particle accessors (read/write via Eigen segment)
    auto q(int i) { return particle_q.segment<3>(i * 3); }
    auto qd(int i) { return particle_qd.segment<3>(i * 3); }
    auto f(int i) { return particle_f.segment<3>(i * 3); }

    auto q(int i) const { return particle_q.segment<3>(i * 3); }
    auto qd(int i) const { return particle_qd.segment<3>(i * 3); }
    auto f(int i) const { return particle_f.segment<3>(i * 3); }

    void clear_forces() { particle_f.setZero(); }
};

} // namespace physics
