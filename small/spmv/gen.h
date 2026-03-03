#pragma once
#include <cstdlib>
#include <vector>
#include <Eigen/Sparse>

// generate random sparse matrix via eigen
// RowMajor = CSR layout, iterate for COO
inline Eigen::SparseMatrix<float, Eigen::RowMajor> randomSparse(unsigned int rows, unsigned int cols, unsigned int nnz) {
    std::vector<Eigen::Triplet<float>> trips(nnz);
    for (unsigned int i = 0; i < nnz; ++i)
        trips[i] = {(int)(rand() % rows), (int)(rand() % cols), (rand() % 100) / 50.0f};

    Eigen::SparseMatrix<float, Eigen::RowMajor> mat(rows, cols);
    mat.setFromTriplets(trips.begin(), trips.end());
    return mat;
}
