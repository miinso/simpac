# Ref bench

Minimal benchmark for `entity.get<T>()` vs `ref.get()` gather cost.

Files:
- `small/ref/bench_ref.cpp`
- `small/ref/BUILD`

Run:

```bash
bazel run //small/ref:ref_bench -c opt
```

What to look for:
- `BM_EntityGet` vs `BM_RefGet` speedup (same springs, same RNG, same access pattern)
- Sensitivity to particle count (4k / 64k / 262k) with fixed spring count

Notes:
- `ref` is cached per spring at build time; this isolates `get()` lookup overhead from the gather itself.
- Adjust `Args` in `small/ref/bench_ref.cpp` to match your typical spring/particle ratio.

Result:
```
INFO: Running command line: bazel-bin/small/ref/ref_bench.exe
2026-01-28T15:39:02+09:00
Running C:\users\user\_bazel_user\vj36eseu\execroot\_main\bazel-out\x64_windows-opt\bin\small\ref\ref_bench.exe
Run on (16 X 3194 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x8)
  L1 Instruction 32 KiB (x8)
  L2 Unified 512 KiB (x8)
  L3 Unified 16384 KiB (x1)
--------------------------------------------------------------------------------------
Benchmark                            Time             CPU   Iterations UserCounters...
--------------------------------------------------------------------------------------
BM_EntityGet/4096/2000000     25089725 ns     24553571 ns           28 items_per_second=81.4545M/s
BM_EntityGet/65536/2000000    48242371 ns     49107143 ns           14 items_per_second=40.7273M/s
BM_EntityGet/262144/2000000   61063731 ns     60096154 ns           13 items_per_second=33.28M/s
BM_RefGet/4096/2000000        11234913 ns     11160714 ns           56 items_per_second=179.2M/s
BM_RefGet/65536/2000000       18012390 ns     17911585 ns           41 items_per_second=111.66M/s
BM_RefGet/262144/2000000      18441610 ns     18292683 ns           41 items_per_second=109.333M/s
```

REMARK: impressive..

TODO: try full-blown comparison like putting every component as ref members

TODO: test with hexahedral (six entity members in a struct)

TODO: find out if i can avoid gather at all (sounds like a perpertual machine tho..)