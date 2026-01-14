# OpenMP and TBB Testing

> FYI: `-c opt` equivalents to `Release` (mostly)

## bench

### Build and Run
```bash
# Native
bazel run //small/openmp-test:bench -c opt

# WASM
bazel build //small/openmp-test:bench_wasm --config=wasm -c opt
node small/openmp-test/run_bench.js
```

### Native Output
```
OMP threads: 16
TBB threads: 16
2026-01-14T16:37:32+09:00
Running C:\users\user\_bazel_user\vj36eseu\execroot\_main\bazel-out\x64_windows-opt\bin\small\openmp-test\bench.exe
Run on (16 X 3194 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x8)
  L1 Instruction 32 KiB (x8)
  L2 Unified 512 KiB (x8)
  L3 Unified 16384 KiB (x1)
---------------------------------------------------------------
Benchmark                     Time             CPU   Iterations
---------------------------------------------------------------
BM_SerialSum/100000       68544 ns        68359 ns        11200
BM_SerialSum/1000000     678377 ns       669643 ns         1120
BM_OMPSum/100000           8279 ns         7812 ns        64000
BM_OMPSum/1000000         73442 ns        69754 ns        11200
BM_TBBSum/100000          17808 ns        17439 ns        44800
BM_TBBSum/1000000         76783 ns        76730 ns         8960
```

### WASM Output
```
OMP threads: 16
TBB threads: 16
***WARNING*** Failed to set thread affinity. Estimated CPU frequency may be incorrect.
2026-01-14T16:36:29+09:00
Running C:/Users/user/lab/simpac/small/openmp-test/run_bench.js
Run on (16 X 999.993 MHz CPU s)
---------------------------------------------------------------
Benchmark                     Time             CPU   Iterations
---------------------------------------------------------------
BM_SerialSum/100000       67644 ns        67645 ns        10040
BM_SerialSum/1000000     685075 ns       685081 ns         1005
BM_OMPSum/100000         259694 ns       259696 ns         2383
BM_OMPSum/1000000        317806 ns       317808 ns         2144
BM_TBBSum/100000          23042 ns        23042 ns        28966
BM_TBBSum/1000000         83703 ns        83704 ns         7346
```

## race

### Build and Run
```bash
# Native
bazel run //small/openmp-test:race -c opt

# WASM
bazel build //small/openmp-test:race_wasm --config=wasm -c opt
node small/openmp-test/run_race.js
```

### Native Output
```
OMP threads: 16
TBB threads: 16
Serial: 0 (thread 0)
Serial: 1 (thread 0)
Serial: 2 (thread 0)
Serial: 3 (thread 0)
Serial: 4 (thread 0)
Serial: 5 (thread 0)
Serial: 6 (thread 0)
Serial: 7 (thread 0)

OpenMP: 7 (thread 7)
OpenMP: 3 (thread 3)
OpenMP: 5 (thread 5)
OpenMP: 0 (thread 0)
OpenMP: 4 (thread 4)
OpenMP: 2 (thread 2)
OpenMP: 6 (thread 6)
OpenMP: 1 (thread 1)

TBB: 0 (thread 0)
TBB: 1 (thread 10)
TBB: 6 (thread 4)
TBB: 7 (thread 8)
TBB: 5 (thread 7)
TBB: 2 (thread 6)
TBB: 3 (thread 5)
TBB: 4 (thread 3)
```

### WASM Output
```
OMP threads: 16
TBB threads: 16
Serial: 0 (thread 0)
Serial: 1 (thread 0)
Serial: 2 (thread 0)
Serial: 3 (thread 0)
Serial: 4 (thread 0)
Serial: 5 (thread 0)
Serial: 6 (thread 0)
Serial: 7 (thread 0)

OpenMP: 0 (thread 0)
OpenMP: 1 (thread 1)
OpenMP: 2 (thread 2)
OpenMP: 3 (thread 3)
OpenMP: 4 (thread 4)
OpenMP: 5 (thread 5)
OpenMP: 6 (thread 6)
OpenMP: 7 (thread 7)

TBB: 0 (thread 0)
TBB: 4 (thread 3)
TBB: 2 (thread 2)
TBB: 1 (thread 4)
TBB: 6 (thread 13)
TBB: 3 (thread 11)
TBB: 5 (thread 1)
TBB: 7 (thread 8)
```

## Dependencies

- OpenMP: native compiler support, simpleomp for WASM
- TBB: onetbb (OneTBB 2022.1.0)
- Google Benchmark: 1.9.4
