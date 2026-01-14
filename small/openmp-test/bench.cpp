#include <benchmark/benchmark.h>
#include <vector>

#ifdef __EMSCRIPTEN__
    #include "simpleomp.h" // for browser
#else
    #include <omp.h>
#endif

static void BM_SerialSum(benchmark::State& state) {
    const int N = state.range(0);
    std::vector<double> data(N, 1.5);

    for (auto _ : state) {
        double sum = 0.0;
        for (int i = 0; i < N; i++) {
            sum += data[i] * data[i];
        }
        benchmark::DoNotOptimize(sum);
    }
}

static void BM_ParallelSum(benchmark::State& state) {
    const int N = state.range(0);
    std::vector<double> data(N, 1.5);

    for (auto _ : state) {
        double sum = 0.0;
        #pragma omp parallel for reduction(+:sum)
        for (int i = 0; i < N; i++) {
            sum += data[i] * data[i];
        }
        benchmark::DoNotOptimize(sum);
    }
}

BENCHMARK(BM_SerialSum)->Arg(100000)->Arg(1000000);
BENCHMARK(BM_ParallelSum)->Arg(100000)->Arg(1000000);

BENCHMARK_MAIN();
