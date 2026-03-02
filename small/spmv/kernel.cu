// from: https://www.youtube.com/watch?v=H6YGKNukGMo
// Izzat El Hajj, CMPS 224/396AA GPU Computing

#include "common.h"
#include "timer.h"

__global__ void spmv_coo_kernel(COOMatrix cooMatrix, float* inVector, float* outVector) {
    unsigned int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < cooMatrix.numNonZeros) {
        unsigned int row = cooMatrix.rowIdxs[i];
        unsigned int col = cooMatrix.colIdxs[i];
        float value = cooMatrix.values[i];
        atomicAdd(&outVector[row], inVector[col] * value);
    }
}

void spmv_coo_gpu(COOMatrix cooMatrix, float* inVector, float* outVector) {
    Timer timer;

    // allocate gpu memory
    startTime(&timer);
    COOMatrix cooMatrix_d;
    cooMatrix_d.numRows = cooMatrix.numRows;
    cooMatrix_d.numCols = cooMatrix.numCols;
    cooMatrix_d.numNonZeros = cooMatrix.numNonZeros;
    cudaMalloc((void **) &cooMatrix_d.rowIdxs, cooMatrix_d.numNonZeros * sizeof(unsigned int));
    cudaMalloc((void **) &cooMatrix_d.colIdxs, cooMatrix_d.numNonZeros * sizeof(unsigned int));
    cudaMalloc((void **) &cooMatrix_d.values, cooMatrix_d.numNonZeros * sizeof(float));
    float* inVector_d;
    cudaMalloc((void **) &inVector_d, cooMatrix_d.numCols * sizeof(float));
    float* outVector_d;
    cudaMalloc((void **) &outVector_d, cooMatrix_d.numRows * sizeof(float));
    cudaDeviceSynchronize();
    stopTime(&timer);
    printElapsedTime(timer, "Allocation time");
    
    // copy data to gpu
    startTime(&timer);
    cudaMemcpy(cooMatrix_d.rowIdxs, cooMatrix.rowIdxs, cooMatrix_d.numNonZeros * sizeof(unsigned int), cudaMemcpyHostToDevice);
    cudaMemcpy(cooMatrix_d.colIdxs, cooMatrix.colIdxs, cooMatrix_d.numNonZeros * sizeof(unsigned int), cudaMemcpyHostToDevice);
    cudaMemcpy(cooMatrix_d.values, cooMatrix.values, cooMatrix_d.numNonZeros * sizeof(float), cudaMemcpyHostToDevice);
    cudaMemcpy(inVector_d, inVector, cooMatrix_d.numCols * sizeof(float), cudaMemcpyHostToDevice);
    cudaMemset(outVector_d, 0, cooMatrix_d.numRows * sizeof(float));
    cudaDeviceSynchronize();
    stopTime(&timer);
    printElapsedTime(timer, "Copy to GPU time");

    // call kernel
    startTime(&timer);
    unsigned int numThreadsPerBlock = 1024;
    unsigned int numBlocks = (cooMatrix_d.numNonZeros + numThreadsPerBlock - 1) / numThreadsPerBlock;
    spmv_coo_kernel <<<numBlocks, numThreadsPerBlock>>> (cooMatrix_d, inVector_d, outVector_d);
    cudaDeviceSynchronize();
    stopTime(&timer);
    printElapsedTime(timer, "Kernel time", GREEN);

    // copy data from gpu
    startTime(&timer);
    cudaMemcpy(outVector, outVector_d, cooMatrix_d.numRows * sizeof(float), cudaMemcpyDeviceToHost);
    cudaDeviceSynchronize();
    stopTime(&timer);
    printElapsedTime(timer, "Copy from GPU time");

    // free gpu memory
    startTime(&timer);
    cudaFree(cooMatrix_d.rowIdxs);
    cudaFree(cooMatrix_d.colIdxs);
    cudaFree(cooMatrix_d.values);
    cudaFree(inVector_d);
    cudaFree(outVector_d);
    cudaDeviceSynchronize();
    stopTime(&timer);
    printElapsedTime(timer, "Deallocation time");
}
