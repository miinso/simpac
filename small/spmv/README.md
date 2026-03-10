# SpMV

CUDA implementations of SpMV in three sparse formats, following
[Izzat El Hajj's GPU Computing lectures](https://www.youtube.com/watch?v=H6YGKNukGMo).

## Formats

| Format | Storage | GPU strategy |
|--------|---------|--------------|
| **COO** | `rowIdxs[nnz]`, `colIdxs[nnz]`, `values[nnz]` | 1 thread/nnz, `atomicAdd` to accumulate |
| **CSR** | `rowPtrs[numRows+1]`, `colIdxs[nnz]`, `values[nnz]` | 1 thread/row, variable-length inner loop |
| **ELL** | `colIdxs[rows*maxNnz]`, `values[rows*maxNnz]`, column-major | 1 thread/row, uniform loop, coalesced access |

## Results

> NVIDIA GeForce GTX 1660 Ti (6GB), Driver 561.17, CUDA 12.6

100K x 100K matrix, ~10M nnz, `maxNnzPerRow = 381`.

| | CPU | Kernel | Copy to GPU | GPU total |
|---|---:|---:|---:|---:|
| **COO** | 28.5 ms | 8.9 ms | 26.7 ms | 42.6 ms |
| **CSR** | **8.5 ms** | 7.4 ms | **17.8 ms** | 30.4 ms |
| **ELL** | 20.1 ms | **0.8 ms** | 65.3 ms | 80.3 ms |

### COO

```sh
bazel run small/spmv:coo -c opt
```

```
CPU time                 28.467 ms

Allocation time           5.199 ms
Copy to GPU time         26.713 ms
Kernel time               8.911 ms
Copy from GPU time        0.337 ms
Deallocation time         0.819 ms
GPU time                 42.601 ms
```

### CSR

```sh
bazel run small/spmv:csr -c opt
```

```
CPU time                  8.476 ms

Allocation time           3.654 ms
Copy to GPU time         17.794 ms
Kernel time               7.424 ms
Copy from GPU time        0.252 ms
Deallocation time         0.589 ms
GPU time                 30.351 ms
```

### ELL

```sh
bazel run small/spmv:ell -c opt
```

```
CPU time                 20.399 ms

Allocation time          11.570 ms
Copy to GPU time         63.396 ms
Kernel time               0.801 ms
Copy from GPU time        0.144 ms
Deallocation time         2.467 ms
GPU time                 79.047 ms
```

## Bench

number looked suspicious, so dedicated bench is warranted.

```sh
bazel run //small/spmv:bench -c opt
```

### 50K x 50K, 500K nnz

```
INFO: Running command line: bazel-bin/small/spmv/bench.exe
2026-03-10T11:13:42+09:00
Running C:\users\user\_bazel_user\vj36eseu\execroot\_main\bazel-out\x64_windows-opt\bin\small\spmv\bench.exe
Run on (16 X 3194 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x8)
  L1 Instruction 32 KiB (x8)
  L2 Unified 512 KiB (x8)
  L3 Unified 16384 KiB (x1)
------------------------------------------------------------------------------------
Benchmark                          Time             CPU   Iterations UserCounters...
------------------------------------------------------------------------------------
BM_COO/500000/manual_time       86.5 us          121 us         6087 bytes_per_second=88.2513Gi/s
BM_CSR/500000/manual_time       87.9 us          116 us         7971 bytes_per_second=67.7907Gi/s
BM_ELL/500000/manual_time       48.3 us         82.8 us        14527 bytes_per_second=123.496Gi/s
```

### 500K x 500K, 5M nnz

```
INFO: Running command line: bazel-bin/small/spmv/bench.exe
2026-03-10T11:10:45+09:00
Running C:\users\user\_bazel_user\vj36eseu\execroot\_main\bazel-out\x64_windows-opt\bin\small\spmv\bench.exe
Run on (16 X 3194 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x8)
  L1 Instruction 32 KiB (x8)
  L2 Unified 512 KiB (x8)
  L3 Unified 16384 KiB (x1)
-------------------------------------------------------------------------------------
Benchmark                           Time             CPU   Iterations UserCounters...
-------------------------------------------------------------------------------------
BM_COO/5000000/manual_time       1863 us         1928 us          308 bytes_per_second=40.9352Gi/s
BM_CSR/5000000/manual_time       2640 us         2712 us          265 bytes_per_second=22.5499Gi/s
BM_ELL/5000000/manual_time        253 us          292 us         2780 bytes_per_second=235.661Gi/s
```

numbers still look suspicious (matches lecturer's but still..)

TODO: bench against cusparse
