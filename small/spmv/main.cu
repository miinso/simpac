// from: https://www.youtube.com/watch?v=H6YGKNukGMo
// Izzat El Hajj, CMPS 224/396AA GPU Computing

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <algorithm>
#include <numeric>

#include "common.h"
#include "timer.h"

// cpu reference
void spmv_coo_cpu(COOMatrix cooMatrix, float* inVector, float* outVector) {
    for (unsigned int row = 0; row < cooMatrix.numRows; ++row)
        outVector[row] = 0.0f;
    for (unsigned int i = 0; i < cooMatrix.numNonZeros; ++i)
        outVector[cooMatrix.rowIdxs[i]] += cooMatrix.values[i] * inVector[cooMatrix.colIdxs[i]];
}

// verify gpu result against cpu reference
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

// generate random sparse matrix in COO format
COOMatrix generateRandomCOOMatrix(unsigned int numRows, unsigned int numCols, unsigned int numNonZeros) {
    COOMatrix cooMatrix;
    cooMatrix.numRows = numRows;
    cooMatrix.numCols = numCols;
    cooMatrix.numNonZeros = numNonZeros;
    cooMatrix.rowIdxs = (unsigned int*) malloc(numNonZeros * sizeof(unsigned int));
    cooMatrix.colIdxs = (unsigned int*) malloc(numNonZeros * sizeof(unsigned int));
    cooMatrix.values  = (float*) malloc(numNonZeros * sizeof(float));
    for (unsigned int i = 0; i < numNonZeros; ++i) {
        cooMatrix.rowIdxs[i] = rand() % numRows;
        cooMatrix.colIdxs[i] = rand() % numCols;
        cooMatrix.values[i]  = 1.0f * (rand() % 100) / 50.0f; // [0, 2)
    }
    // sort by row index
    unsigned int* perm = (unsigned int*) malloc(numNonZeros * sizeof(unsigned int));
    std::iota(perm, perm + numNonZeros, 0);
    std::sort(perm, perm + numNonZeros, [&](unsigned int a, unsigned int b) {
        return cooMatrix.rowIdxs[a] < cooMatrix.rowIdxs[b];
    });
    unsigned int* tmpRow = (unsigned int*) malloc(numNonZeros * sizeof(unsigned int));
    unsigned int* tmpCol = (unsigned int*) malloc(numNonZeros * sizeof(unsigned int));
    float* tmpVal = (float*) malloc(numNonZeros * sizeof(float));
    for (unsigned int i = 0; i < numNonZeros; ++i) {
        tmpRow[i] = cooMatrix.rowIdxs[perm[i]];
        tmpCol[i] = cooMatrix.colIdxs[perm[i]];
        tmpVal[i] = cooMatrix.values[perm[i]];
    }
    std::swap(cooMatrix.rowIdxs, tmpRow);
    std::swap(cooMatrix.colIdxs, tmpCol);
    std::swap(cooMatrix.values, tmpVal);
    free(tmpRow); free(tmpCol); free(tmpVal); free(perm);
    return cooMatrix;
}

int main(int argc, char** argv) {
    Timer timer;

    unsigned int numRows = 50000;
    unsigned int numCols = 50000;
    unsigned int numNonZeros = 500000; // about 0.02%
    srand(0);

    // generate random sparse matrix
    COOMatrix cooMatrix = generateRandomCOOMatrix(numRows, numCols, numNonZeros);
    printf("sparse matrix: %u x %u, %u nnz\n\n", numRows, numCols, numNonZeros);

    // generate random input vector
    float* inVector = (float*) malloc(numCols * sizeof(float));
    for (unsigned int i = 0; i < numCols; ++i)
        inVector[i] = 1.0f * (rand() % 100) / 50.0f;

    float* outVector_cpu = (float*) malloc(numRows * sizeof(float));
    float* outVector_gpu = (float*) malloc(numRows * sizeof(float));

    // warm up cuda context
    cudaFree(0); // this alone takes like the whole second, very sad..

    // cpu
    startTime(&timer);
    spmv_coo_cpu(cooMatrix, inVector, outVector_cpu);
    stopTime(&timer);
    printElapsedTime(timer, "CPU time");
    printf("\n");

    // gpu
    startTime(&timer);
    spmv_coo_gpu(cooMatrix, inVector, outVector_gpu);
    stopTime(&timer);
    printElapsedTime(timer, "GPU time", GREEN);
    printf("\n");

    // verify
    printf("verify: %s\n", verify(outVector_cpu, outVector_gpu, numRows) ? "PASS" : "FAIL");

    free(cooMatrix.rowIdxs);
    free(cooMatrix.colIdxs);
    free(cooMatrix.values);
    free(inVector);
    free(outVector_cpu);
    free(outVector_gpu);

    return 0;
}
