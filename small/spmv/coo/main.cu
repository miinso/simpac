// from: https://www.youtube.com/watch?v=H6YGKNukGMo
// Izzat El Hajj, CMPS 224/396AA GPU Computing

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <Eigen/Sparse>

#include "common.h"
#include "small/spmv/timer.h"
#include "small/spmv/gen.h"

// cpu reference
void spmv_coo_cpu(COOMatrix cooMatrix, float* inVector, float* outVector) {
    for (unsigned int row = 0; row < cooMatrix.numRows; ++row)
        outVector[row] = 0.0f;
    for (unsigned int i = 0; i < cooMatrix.numNonZeros; ++i)
        outVector[cooMatrix.rowIdxs[i]] += cooMatrix.values[i] * inVector[cooMatrix.colIdxs[i]];
}

int verify(float* cpu, float* gpu, unsigned int n) {
    const float eps = 1e-3f;
    for (unsigned int i = 0; i < n; ++i) {
        if (fabsf(cpu[i] - gpu[i]) > eps) {
            printf("mismatch at %u: cpu=%.6f gpu=%.6f\n", i, cpu[i], gpu[i]);
            return 0;
        }
    }
    return 1;
}

int main(int argc, char** argv) {
    Timer timer;
    srand(0);

    auto mat = randomSparse(100000, 100000, 10000000); // 10M nnz
    unsigned int nnz = mat.nonZeros();

    // extract COO from eigen
    COOMatrix cooMatrix;
    cooMatrix.numRows = mat.rows();
    cooMatrix.numCols = mat.cols();
    cooMatrix.numNonZeros = nnz;
    cooMatrix.rowIdxs = (unsigned int*) malloc(nnz * sizeof(unsigned int));
    cooMatrix.colIdxs = (unsigned int*) malloc(nnz * sizeof(unsigned int));
    cooMatrix.values  = (float*) malloc(nnz * sizeof(float));
    unsigned int idx = 0;
    for (int k = 0; k < mat.outerSize(); ++k)
        for (Eigen::SparseMatrix<float, Eigen::RowMajor>::InnerIterator it(mat, k); it; ++it, ++idx) {
            cooMatrix.rowIdxs[idx] = it.row();
            cooMatrix.colIdxs[idx] = it.col();
            cooMatrix.values[idx]  = it.value();
        }

    printf("sparse matrix: %u x %u, %u nnz\n\n", cooMatrix.numRows, cooMatrix.numCols, nnz);

    Eigen::VectorXf vec = Eigen::VectorXf::Random(mat.cols());
    float* outVector_cpu = (float*) malloc(mat.rows() * sizeof(float));
    float* outVector_gpu = (float*) malloc(mat.rows() * sizeof(float));

    // warm up cuda context
    cudaFree(0); // this alone takes like the whole second, very sad..

    // cpu
    startTime(&timer);
    spmv_coo_cpu(cooMatrix, vec.data(), outVector_cpu);
    stopTime(&timer);
    printElapsedTime(timer, "CPU time");
    printf("\n");

    // gpu
    startTime(&timer);
    spmv_coo_gpu(cooMatrix, vec.data(), outVector_gpu);
    stopTime(&timer);
    printElapsedTime(timer, "GPU time", GREEN);
    printf("\n");

    printf("verify: %s\n", verify(outVector_cpu, outVector_gpu, mat.rows()) ? "PASS" : "FAIL");

    free(cooMatrix.rowIdxs); free(cooMatrix.colIdxs); free(cooMatrix.values);
    free(outVector_cpu); free(outVector_gpu);
    return 0;
}
