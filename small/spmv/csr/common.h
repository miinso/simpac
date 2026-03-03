#pragma once

struct CSRMatrix {
    unsigned int numRows;
    unsigned int numCols;
    unsigned int numNonZeros;
    unsigned int* rowPtrs;  // size: numRows + 1
    unsigned int* colIdxs;
    float* values;
};

__global__ void spmv_csr_kernel(CSRMatrix csrMatrix, float* inVector, float* outVector);
void spmv_csr_gpu(CSRMatrix csrMatrix, float* inVector, float* outVector);
