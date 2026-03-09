// from: https://www.youtube.com/watch?v=bDbUoRrT6Js
// Izzat El Hajj, CMPS 224/396AA GPU Computing

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <Eigen/Sparse>

#include "common.h"
#include "small/spmv/timer.h"
#include "small/spmv/gen.h"

// cpu reference (column-major layout, matching GPU kernel)
void spmv_ell_cpu(ELLMatrix ellMatrix, float* inVector, float* outVector) {
    for (unsigned int row = 0; row < ellMatrix.numRows; ++row) {
        float sum = 0.0f;
        for (unsigned int j = 0; j < ellMatrix.nnzPerRow[row]; ++j) {
            unsigned int idx = j * ellMatrix.numRows + row; // column-major
            sum += ellMatrix.values[idx] * inVector[ellMatrix.colIdxs[idx]];
        }
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
    unsigned int rows = mat.rows(), cols = mat.cols(), nnz = mat.nonZeros();
    unsigned int* nnzPerRow = (unsigned int*) malloc(rows * sizeof(unsigned int));
    // find max nnz per row
    unsigned int maxNnzPerRow = 0;
    for (int k = 0; k < (int)rows; ++k) {
        unsigned int rowLen = mat.outerIndexPtr()[k + 1] - mat.outerIndexPtr()[k];
        nnzPerRow[k] = rowLen;
        if (rowLen > maxNnzPerRow) maxNnzPerRow = rowLen;
    }

    // convert CSR (eigen RowMajor) → ELL
    ELLMatrix ellMatrix;
    ellMatrix.numRows = rows;
    ellMatrix.numCols = cols;
    // ellMatrix.numNonZeros = nnz;
    ellMatrix.nnzPerRow = nnzPerRow;
    ellMatrix.maxNnzPerRow = maxNnzPerRow;
    size_t ellSize = (size_t)rows * maxNnzPerRow;
    ellMatrix.colIdxs = (unsigned int*) malloc(ellSize * sizeof(unsigned int));
    ellMatrix.values  = (float*) malloc(ellSize * sizeof(float));
    memset(ellMatrix.colIdxs, 0xFF, ellSize * sizeof(unsigned int)); // sentinel = -1
    memset(ellMatrix.values, 0, ellSize * sizeof(float));

    const int* rowPtrs = mat.outerIndexPtr();
    const int* colIdxs = mat.innerIndexPtr();
    const float* values = mat.valuePtr();
    for (unsigned int row = 0; row < rows; ++row) {
        unsigned int j = 0;
        for (int i = rowPtrs[row]; i < rowPtrs[row + 1]; ++i, ++j) {
            ellMatrix.colIdxs[j * rows + row] = colIdxs[i];  // column-major
            ellMatrix.values[j * rows + row]  = values[i];
        }
    }

    printf("sparse matrix: %u x %u, %u nnz, maxNnzPerRow: %u\n\n",
        rows, cols, nnz, maxNnzPerRow);

    Eigen::VectorXf vec = Eigen::VectorXf::Random(cols);
    float* outVector_cpu = (float*) malloc(rows * sizeof(float));
    float* outVector_gpu = (float*) malloc(rows * sizeof(float));

    // warm up cuda context
    cudaFree(0);

    // cpu
    startTime(&timer);
    spmv_ell_cpu(ellMatrix, vec.data(), outVector_cpu);
    stopTime(&timer);
    printElapsedTime(timer, "CPU time");
    printf("\n");

    // gpu
    startTime(&timer);
    spmv_ell_gpu(ellMatrix, vec.data(), outVector_gpu);
    stopTime(&timer);
    printElapsedTime(timer, "GPU time", GREEN);
    printf("\n");

    printf("verify: %s\n", verify(outVector_cpu, outVector_gpu, rows) ? "PASS" : "FAIL");

    free(ellMatrix.colIdxs); free(ellMatrix.values); free(nnzPerRow);
    free(outVector_cpu); free(outVector_gpu);
    return 0;
}
