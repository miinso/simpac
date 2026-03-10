// spmv benchmark: coo vs csr vs ell vs (maybe cusparse later..)
// uses google benchmark with cudaEvent manual timing

#include <cstdlib>
#include <vector>
#include <cuda_runtime.h>
#include <benchmark/benchmark.h>
#include <Eigen/Sparse>

#include "coo/common.h"
#include "csr/common.h"
#include "ell/common.h"
#include "gen.h"

// ---- shared data ----

static const unsigned int ROWS = 50000, COLS = 50000;

static Eigen::SparseMatrix<float, Eigen::RowMajor> g_mat;
static Eigen::VectorXf g_vec;
static bool g_initialized = false;

void initData(unsigned int nnz) {
    if (g_initialized) return;
    g_initialized = true;
    srand(0);
    g_mat = randomSparse(ROWS, COLS, nnz);
    g_vec = Eigen::VectorXf::Random(COLS);
}

// ---- benchmarks ----

static void BM_COO(benchmark::State& state) {
    initData(state.range(0));
    unsigned int nnz = g_mat.nonZeros();

    // extract coo from eigen (iterate compressed matrix)
    std::vector<unsigned int> rowIdxs(nnz), colIdxs(nnz);
    std::vector<float> values(nnz);
    unsigned int idx = 0;
    for (int k = 0; k < g_mat.outerSize(); ++k)
        for (Eigen::SparseMatrix<float, Eigen::RowMajor>::InnerIterator it(g_mat, k); it; ++it, ++idx) {
            rowIdxs[idx] = it.row();
            colIdxs[idx] = it.col();
            values[idx]  = it.value();
        }

    COOMatrix coo_d;
    coo_d.numRows = ROWS; coo_d.numCols = COLS; coo_d.numNonZeros = nnz;
    cudaMalloc(&coo_d.rowIdxs, nnz * sizeof(unsigned int));
    cudaMalloc(&coo_d.colIdxs, nnz * sizeof(unsigned int));
    cudaMalloc(&coo_d.values, nnz * sizeof(float));
    float *in_d, *out_d;
    cudaMalloc(&in_d, COLS * sizeof(float));
    cudaMalloc(&out_d, ROWS * sizeof(float));

    cudaMemcpy(coo_d.rowIdxs, rowIdxs.data(), nnz * sizeof(unsigned int), cudaMemcpyHostToDevice);
    cudaMemcpy(coo_d.colIdxs, colIdxs.data(), nnz * sizeof(unsigned int), cudaMemcpyHostToDevice);
    cudaMemcpy(coo_d.values, values.data(), nnz * sizeof(float), cudaMemcpyHostToDevice);
    cudaMemcpy(in_d, g_vec.data(), COLS * sizeof(float), cudaMemcpyHostToDevice);

    unsigned int blocks = (nnz + 1023) / 1024;
    cudaEvent_t start, stop;
    cudaEventCreate(&start);
    cudaEventCreate(&stop);

    for (auto _ : state) {
        cudaMemset(out_d, 0, ROWS * sizeof(float));
        cudaEventRecord(start);
        spmv_coo_kernel<<<blocks, 1024>>>(coo_d, in_d, out_d);
        cudaEventRecord(stop);
        cudaEventSynchronize(stop);
        float ms = 0;
        cudaEventElapsedTime(&ms, start, stop);
        state.SetIterationTime(ms / 1000.0);
    }

    size_t bytes = nnz * (sizeof(unsigned int) * 2 + sizeof(float) + sizeof(float)) + ROWS * sizeof(float);
    state.SetBytesProcessed(state.iterations() * bytes);

    cudaEventDestroy(start); cudaEventDestroy(stop);
    cudaFree(coo_d.rowIdxs); cudaFree(coo_d.colIdxs); cudaFree(coo_d.values);
    cudaFree(in_d); cudaFree(out_d);
}

static void BM_CSR(benchmark::State& state) {
    initData(state.range(0));
    unsigned int nnz = g_mat.nonZeros();

    // eigen RowMajor sparse matrix IS csr — just cast the pointers
    const int* outerPtr = g_mat.outerIndexPtr();
    const int* innerPtr = g_mat.innerIndexPtr();
    const float* valPtr = g_mat.valuePtr();

    // eigen uses int, our struct uses unsigned int — copy to match
    std::vector<unsigned int> rowPtrs(outerPtr, outerPtr + ROWS + 1);
    std::vector<unsigned int> colIdxs(innerPtr, innerPtr + nnz);

    CSRMatrix csr_d;
    csr_d.numRows = ROWS; csr_d.numCols = COLS; csr_d.numNonZeros = nnz;
    cudaMalloc(&csr_d.rowPtrs, (ROWS + 1) * sizeof(unsigned int));
    cudaMalloc(&csr_d.colIdxs, nnz * sizeof(unsigned int));
    cudaMalloc(&csr_d.values, nnz * sizeof(float));
    float *in_d, *out_d;
    cudaMalloc(&in_d, COLS * sizeof(float));
    cudaMalloc(&out_d, ROWS * sizeof(float));

    cudaMemcpy(csr_d.rowPtrs, rowPtrs.data(), (ROWS + 1) * sizeof(unsigned int), cudaMemcpyHostToDevice);
    cudaMemcpy(csr_d.colIdxs, colIdxs.data(), nnz * sizeof(unsigned int), cudaMemcpyHostToDevice);
    cudaMemcpy(csr_d.values, valPtr, nnz * sizeof(float), cudaMemcpyHostToDevice);
    cudaMemcpy(in_d, g_vec.data(), COLS * sizeof(float), cudaMemcpyHostToDevice);

    unsigned int blocks = (ROWS + 1023) / 1024;
    cudaEvent_t start, stop;
    cudaEventCreate(&start);
    cudaEventCreate(&stop);

    for (auto _ : state) {
        cudaEventRecord(start);
        spmv_csr_kernel<<<blocks, 1024>>>(csr_d, in_d, out_d);
        cudaEventRecord(stop);
        cudaEventSynchronize(stop);
        float ms = 0;
        cudaEventElapsedTime(&ms, start, stop);
        state.SetIterationTime(ms / 1000.0);
    }

    size_t bytes = (ROWS + 1) * sizeof(unsigned int) + nnz * (sizeof(unsigned int) + sizeof(float) + sizeof(float))
                 + ROWS * sizeof(float);
    state.SetBytesProcessed(state.iterations() * bytes);

    cudaEventDestroy(start); cudaEventDestroy(stop);
    cudaFree(csr_d.rowPtrs); cudaFree(csr_d.colIdxs); cudaFree(csr_d.values);
    cudaFree(in_d); cudaFree(out_d);
}

static void BM_ELL(benchmark::State& state) {
    initData(state.range(0));
    unsigned int nnz = g_mat.nonZeros();

    // find max nnz per row
    unsigned int maxNnzPerRow = 0;
    std::vector<unsigned int> nnzPerRow(ROWS);
    for (unsigned int k = 0; k < ROWS; ++k) {
        unsigned int len = g_mat.outerIndexPtr()[k + 1] - g_mat.outerIndexPtr()[k];
        nnzPerRow[k] = len;
        if (len > maxNnzPerRow) maxNnzPerRow = len;
    }

    // convert CSR → ELL (column-major)
    size_t ellSize = (size_t)ROWS * maxNnzPerRow;
    std::vector<unsigned int> colIdxs(ellSize, (unsigned int)-1);
    std::vector<float> values(ellSize, 0.0f);

    const int* rowPtrs = g_mat.outerIndexPtr();
    const int* innerIdx = g_mat.innerIndexPtr();
    const float* valPtr = g_mat.valuePtr();
    for (unsigned int row = 0; row < ROWS; ++row) {
        unsigned int j = 0;
        for (int i = rowPtrs[row]; i < rowPtrs[row + 1]; ++i, ++j) {
            colIdxs[j * ROWS + row] = innerIdx[i];  // column-major
            values[j * ROWS + row]  = valPtr[i];
        }
    }

    // allocate device
    ELLMatrix ell_d;
    ell_d.numRows = ROWS; ell_d.numCols = COLS; ell_d.maxNnzPerRow = maxNnzPerRow;
    cudaMalloc(&ell_d.nnzPerRow, ROWS * sizeof(unsigned int));
    cudaMalloc(&ell_d.colIdxs, ellSize * sizeof(unsigned int));
    cudaMalloc(&ell_d.values, ellSize * sizeof(float));
    float *in_d, *out_d;
    cudaMalloc(&in_d, COLS * sizeof(float));
    cudaMalloc(&out_d, ROWS * sizeof(float));

    cudaMemcpy(ell_d.nnzPerRow, nnzPerRow.data(), ROWS * sizeof(unsigned int), cudaMemcpyHostToDevice);
    cudaMemcpy(ell_d.colIdxs, colIdxs.data(), ellSize * sizeof(unsigned int), cudaMemcpyHostToDevice);
    cudaMemcpy(ell_d.values, values.data(), ellSize * sizeof(float), cudaMemcpyHostToDevice);
    cudaMemcpy(in_d, g_vec.data(), COLS * sizeof(float), cudaMemcpyHostToDevice);

    unsigned int blocks = (ROWS + 1023) / 1024;
    cudaEvent_t start, stop;
    cudaEventCreate(&start);
    cudaEventCreate(&stop);

    for (auto _ : state) {
        cudaEventRecord(start);
        spmv_ell_kernel<<<blocks, 1024>>>(ell_d, in_d, out_d);
        cudaEventRecord(stop);
        cudaEventSynchronize(stop);
        float ms = 0;
        cudaEventElapsedTime(&ms, start, stop);
        state.SetIterationTime(ms / 1000.0);
    }

    // bytes: nnzPerRow + colIdxs + values + inVector + outVector
    size_t bytes = ROWS * sizeof(unsigned int)
                 + nnz * (sizeof(unsigned int) + sizeof(float) + sizeof(float))
                 + ROWS * sizeof(float);
    state.SetBytesProcessed(state.iterations() * bytes);

    cudaEventDestroy(start); cudaEventDestroy(stop);
    cudaFree(ell_d.nnzPerRow); cudaFree(ell_d.colIdxs); cudaFree(ell_d.values);
    cudaFree(in_d); cudaFree(out_d);
}

BENCHMARK(BM_COO)->Arg(500000)->UseManualTime()->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_CSR)->Arg(500000)->UseManualTime()->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_ELL)->Arg(500000)->UseManualTime()->Unit(benchmark::kMicrosecond);

BENCHMARK_MAIN();
