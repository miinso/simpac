#pragma once

#include "base.h"

#include <deque>
#include <string>
#include <vector>

struct Solver {
    Eigen::SparseMatrix<Real> A;
    Eigen::VectorXr b;
    Eigen::VectorXr x;
    Eigen::VectorXr x_prev;

    std::vector<Eigen::Triplet<Real>> triplets;

    bool exploded = false;

    int cg_iterations = 0;
    Real cg_error = 0;

    std::deque<std::string> cg_history;
    int cg_history_max_lines = 15;

    Eigen::ConjugateGradient<Eigen::SparseMatrix<Real>> cg;
};

namespace components {

inline void register_solver_component(flecs::world& ecs) {
    ecs.component<Solver>()
        .add(flecs::Singleton);
}

} // namespace components
