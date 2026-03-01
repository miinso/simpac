// osqp demo from: https://osqp.org/docs/examples/setup-and-solve.html
//
//      y
// 0.1   \
//        \
// 0.7 +---*----+     * = solution (.3, .7)
//     |    \   |
//     |     \  |     x + y = 1, 0 <= x,y <= .7
//     |      \ |
// 0.0 +-------\+
//     0.0   0.7 1.0  x

#include <cstdio>
#include <Eigen/Dense>
#include <Eigen/Sparse>
#include <OsqpEigen/OsqpEigen.h>

// bcr osqp builds with PROFILING but has no wasm timer impl
// TODO: submit upstream patch later
#ifdef __EMSCRIPTEN__
#include <cstdlib>
extern "C" {
struct OSQPTimer { int dummy; };
OSQPTimer* OSQPTimer_new(void) { return (OSQPTimer*)calloc(1, sizeof(OSQPTimer)); }
void OSQPTimer_free(OSQPTimer* t) { free(t); }
void osqp_tic(OSQPTimer* t) { (void)t; }
double osqp_toc(OSQPTimer* t) { (void)t; return 0.0; }
}
#endif

int main() {
    // P = [4 1; 1 2], q = [1; 1]
    Eigen::SparseMatrix<double> P(2, 2);
    P.insert(0, 0) = 4;  P.insert(0, 1) = 1;
    P.insert(1, 0) = 1;  P.insert(1, 1) = 2;
    Eigen::Vector2d q(1, 1);

    // constraints:
    // x + y = 1
    // 0 <= x <= 0.7
    // 0 <= y <= 0.7

    // A = [1 1; 1 0; 0 1], l = [1 0 0], u = [1 0.7 0.7]
    Eigen::SparseMatrix<double> A(3, 2);
    A.insert(0, 0) = 1;  A.insert(0, 1) = 1;
    A.insert(1, 0) = 1;
    A.insert(2, 1) = 1;
    Eigen::Vector3d lo(1, 0, 0), hi(1, 0.7, 0.7);

    OsqpEigen::Solver solver;
    solver.settings()->setVerbosity(false);
    // solver.settings()->setVerbosity(true);
    solver.data()->setNumberOfVariables(2);
    solver.data()->setNumberOfConstraints(3);
    solver.data()->setHessianMatrix(P);
    solver.data()->setGradient(q);
    solver.data()->setLinearConstraintsMatrix(A);
    solver.data()->setLowerBound(lo);
    solver.data()->setUpperBound(hi);

    solver.initSolver();
    solver.solveProblem();

    auto x = solver.getSolution();
    printf("x = (%.4f, %.4f)\n", x(0), x(1));
    printf("status: %s\n", solver.getStatus() == OsqpEigen::Status::Solved ? "solved" : "failed");
}

// INFO: Running command line: bazel-bin/small/qp/main.exe
// x = (0.3014, 0.6984)
// status: solved
