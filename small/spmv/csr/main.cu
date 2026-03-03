// from: https://www.youtube.com/watch?v=H6YGKNukGMo
// Izzat El Hajj, CMPS 224/396AA GPU Computing

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <Eigen/Sparse>

#include "common.h"
#include "small/spmv/timer.h"
#include "small/spmv/gen.h"

// cpu reference
void spmv_csr_cpu(CSRMatrix csrMatrix, float* inVector, float* outVector) {
    for (unsigned int row = 0; row < csrMatrix.numRows; ++row) {
        float sum = 0.0f;
        for (unsigned int i = csrMatrix.rowPtrs[row]; i < csrMatrix.rowPtrs[row + 1]; ++i)
            sum += csrMatrix.values[i] * inVector[csrMatrix.colIdxs[i]];
        outVector[row] = sum;
    }
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

    // eigen RowMajor IS csr — just cast pointers
    std::vector<unsigned int> rowPtrs(mat.outerIndexPtr(), mat.outerIndexPtr() + mat.rows() + 1);
    std::vector<unsigned int> colIdxs(mat.innerIndexPtr(), mat.innerIndexPtr() + nnz);

    CSRMatrix csrMatrix;
    csrMatrix.numRows = mat.rows();
    csrMatrix.numCols = mat.cols();
    csrMatrix.numNonZeros = nnz;
    csrMatrix.rowPtrs = rowPtrs.data();
    csrMatrix.colIdxs = colIdxs.data();
    csrMatrix.values  = const_cast<float*>(mat.valuePtr());

    printf("sparse matrix: %u x %u, %u nnz\n\n", csrMatrix.numRows, csrMatrix.numCols, nnz);

    Eigen::VectorXf vec = Eigen::VectorXf::Random(mat.cols());
    float* outVector_cpu = (float*) malloc(mat.rows() * sizeof(float));
    float* outVector_gpu = (float*) malloc(mat.rows() * sizeof(float));

    cudaFree(0);

    // cpu
    startTime(&timer);
    spmv_csr_cpu(csrMatrix, vec.data(), outVector_cpu);
    stopTime(&timer);
    printElapsedTime(timer, "CPU time");
    printf("\n");

    // gpu
    startTime(&timer);
    spmv_csr_gpu(csrMatrix, vec.data(), outVector_gpu);
    stopTime(&timer);
    printElapsedTime(timer, "GPU time", GREEN);
    printf("\n");

    printf("verify: %s\n", verify(outVector_cpu, outVector_gpu, mat.rows()) ? "PASS" : "FAIL");

    free(outVector_cpu); free(outVector_gpu);
    return 0;
}
