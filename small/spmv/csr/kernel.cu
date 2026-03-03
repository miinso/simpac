// from: https://www.youtube.com/watch?v=H6YGKNukGMo
// Izzat El Hajj, CMPS 224/396AA GPU Computing

#include "common.h"
#include "small/spmv/timer.h"

__global__ void spmv_csr_kernel(CSRMatrix csrMatrix, float* inVector, float* outVector) {
    unsigned int row = blockIdx.x * blockDim.x + threadIdx.x;
    if (row < csrMatrix.numRows) {
        float sum = 0.0f;
        for (unsigned int i = csrMatrix.rowPtrs[row]; i < csrMatrix.rowPtrs[row + 1]; ++i) {
            unsigned int col = csrMatrix.colIdxs[i];
            float value = csrMatrix.values[i];
            sum += value * inVector[col];
        }
        outVector[row] = sum;
    }
}

void spmv_csr_gpu(CSRMatrix csrMatrix, float* inVector, float* outVector) {
    Timer timer;

    // allocate gpu memory
    startTime(&timer);
    CSRMatrix csrMatrix_d;
    csrMatrix_d.numRows = csrMatrix.numRows;
    csrMatrix_d.numCols = csrMatrix.numCols;
    csrMatrix_d.numNonZeros = csrMatrix.numNonZeros;
    cudaMalloc((void**) &csrMatrix_d.rowPtrs, (csrMatrix_d.numRows + 1) * sizeof(unsigned int));
    cudaMalloc((void**) &csrMatrix_d.colIdxs, csrMatrix_d.numNonZeros * sizeof(unsigned int));
    cudaMalloc((void**) &csrMatrix_d.values, csrMatrix_d.numNonZeros * sizeof(float));
    float* inVector_d;
    cudaMalloc((void**) &inVector_d, csrMatrix_d.numCols * sizeof(float));
    float* outVector_d;
    cudaMalloc((void**) &outVector_d, csrMatrix_d.numRows * sizeof(float));
    cudaDeviceSynchronize();
    stopTime(&timer);
    printElapsedTime(timer, "Allocation time");

    // copy data to gpu
    startTime(&timer);
    cudaMemcpy(csrMatrix_d.rowPtrs, csrMatrix.rowPtrs, (csrMatrix_d.numRows + 1) * sizeof(unsigned int), cudaMemcpyHostToDevice);
    cudaMemcpy(csrMatrix_d.colIdxs, csrMatrix.colIdxs, csrMatrix_d.numNonZeros * sizeof(unsigned int), cudaMemcpyHostToDevice);
    cudaMemcpy(csrMatrix_d.values, csrMatrix.values, csrMatrix_d.numNonZeros * sizeof(float), cudaMemcpyHostToDevice);
    cudaMemcpy(inVector_d, inVector, csrMatrix_d.numCols * sizeof(float), cudaMemcpyHostToDevice);
    cudaDeviceSynchronize();
    stopTime(&timer);
    printElapsedTime(timer, "Copy to GPU time");

    // call kernel
    startTime(&timer);
    unsigned int numThreadsPerBlock = 1024;
    unsigned int numBlocks = (csrMatrix_d.numRows + numThreadsPerBlock - 1) / numThreadsPerBlock;
    spmv_csr_kernel<<<numBlocks, numThreadsPerBlock>>>(csrMatrix_d, inVector_d, outVector_d);
    cudaDeviceSynchronize();
    stopTime(&timer);
    printElapsedTime(timer, "Kernel time", GREEN);

    // copy data from gpu
    startTime(&timer);
    cudaMemcpy(outVector, outVector_d, csrMatrix_d.numRows * sizeof(float), cudaMemcpyDeviceToHost);
    cudaDeviceSynchronize();
    stopTime(&timer);
    printElapsedTime(timer, "Copy from GPU time");

    // free gpu memory
    startTime(&timer);
    cudaFree(csrMatrix_d.rowPtrs);
    cudaFree(csrMatrix_d.colIdxs);
    cudaFree(csrMatrix_d.values);
    cudaFree(inVector_d);
    cudaFree(outVector_d);
    cudaDeviceSynchronize();
    stopTime(&timer);
    printElapsedTime(timer, "Deallocation time");
}
