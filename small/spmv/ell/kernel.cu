// from: https://www.youtube.com/watch?v=bDbUoRrT6Js
// Izzat El Hajj, CMPS 224/396AA GPU Computing

#include "common.h"
#include "small/spmv/timer.h"

__global__ void spmv_ell_kernel(ELLMatrix ellMatrix, float* inVector, float* outVector) {
    unsigned int row = blockIdx.x * blockDim.x + threadIdx.x;
    if (row < ellMatrix.numRows) {
        float sum = 0.0f;
        for (unsigned int iter = 0; iter < ellMatrix.nnzPerRow[row]; ++iter) {
            unsigned int i = iter * ellMatrix.numRows + row; // expect coalising, bc `i` is `row` driven, and `row` is `threadIdx.x` driven
            unsigned int col = ellMatrix.colIdxs[i]; // for inVector
            float value = ellMatrix.values[i];
            // outVector[row] += inVector[col] * value; // avoid global mem access
            sum += inVector[col] * value;
        }
        outVector[row] = sum;
    }
}

void spmv_ell_gpu(ELLMatrix ellMatrix, float* inVector, float* outVector) {
    Timer timer;

    // allocate gpu memory
    startTime(&timer);
    ELLMatrix ellMatrix_d;
    ellMatrix_d.numRows = ellMatrix.numRows;
    ellMatrix_d.numCols = ellMatrix.numCols;
    ellMatrix_d.maxNnzPerRow = ellMatrix.maxNnzPerRow;
    cudaMalloc((void **) &ellMatrix_d.nnzPerRow, ellMatrix_d.numRows * sizeof(unsigned int));
    cudaMalloc((void **) &ellMatrix_d.colIdxs, ellMatrix_d.numRows * ellMatrix_d.maxNnzPerRow * sizeof(unsigned int));
    cudaMalloc((void **) &ellMatrix_d.values, ellMatrix_d.numRows * ellMatrix_d.maxNnzPerRow * sizeof(float));
    float* inVector_d;
    cudaMalloc((void **) &inVector_d, ellMatrix_d.numCols * sizeof(float));
    float* outVector_d;
    cudaMalloc((void **) &outVector_d, ellMatrix_d.numRows * sizeof(float));
    cudaDeviceSynchronize();
    stopTime(&timer);
    printElapsedTime(timer, "Allocation time");

    // copy data to gpu
    startTime(&timer);
    cudaMemcpy(ellMatrix_d.nnzPerRow, ellMatrix.nnzPerRow, ellMatrix_d.numRows * sizeof(unsigned int), cudaMemcpyHostToDevice);
    cudaMemcpy(ellMatrix_d.colIdxs, ellMatrix.colIdxs, ellMatrix_d.numRows * ellMatrix_d.maxNnzPerRow * sizeof(unsigned int), cudaMemcpyHostToDevice);
    cudaMemcpy(ellMatrix_d.values, ellMatrix.values, ellMatrix_d.numRows * ellMatrix_d.maxNnzPerRow * sizeof(float), cudaMemcpyHostToDevice);
    cudaMemcpy(inVector_d, inVector, ellMatrix_d.numCols * sizeof(float), cudaMemcpyHostToDevice);
    cudaDeviceSynchronize();
    stopTime(&timer);
    printElapsedTime(timer, "Copy to GPU time");

    // call kernel
    startTime(&timer);
    unsigned int numThreadsPerBlock = 1024;
    unsigned int numBlocks = (ellMatrix_d.numRows + numThreadsPerBlock - 1) / numThreadsPerBlock;
    spmv_ell_kernel<<<numBlocks, numThreadsPerBlock>>>(ellMatrix_d, inVector_d, outVector_d);
    cudaDeviceSynchronize();
    stopTime(&timer);
    printElapsedTime(timer, "Kernel time", GREEN);

    // copy data from gpu
    startTime(&timer);
        cudaMemcpy(outVector, outVector_d, ellMatrix_d.numRows * sizeof(float), cudaMemcpyDeviceToHost);
    cudaDeviceSynchronize();
    stopTime(&timer);
    printElapsedTime(timer, "Copy from GPU time");

    // free gpu  memory
    startTime(&timer);
    cudaFree(ellMatrix_d.colIdxs);
    cudaFree(ellMatrix_d.nnzPerRow);
    cudaFree(ellMatrix_d.values);
    cudaFree(inVector_d);
    cudaFree(outVector_d);
    stopTime(&timer);
    printElapsedTime(timer, "Deallocation time");
}
