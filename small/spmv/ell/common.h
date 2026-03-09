#pragma once

struct ELLMatrix {
    unsigned int numRows;
    unsigned int numCols;
    unsigned int maxNnzPerRow; // padded width
    unsigned int* nnzPerRow;
    unsigned int* colIdxs;     // work as a lookup table, the same size as values
    float* values;             // numRows * maxNnzPerRow (row-major, padded)
};

__global__ void spmv_ell_kernel(ELLMatrix ellMatrix, float* inVector, float* outVector);
void spmv_ell_gpu(ELLMatrix ellMatrix, float* inVector, float* outVector);
