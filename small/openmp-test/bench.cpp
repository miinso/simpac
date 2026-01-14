#include <benchmark/benchmark.h>
#include <vector>
#include <atomic>
#include <thread>
#include <tbb/parallel_reduce.h>
#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>
#include <tbb/task_arena.h>
#include <tbb/partitioner.h>

#ifdef __EMSCRIPTEN__
    #include "simpleomp.h"
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

static void BM_OMPSum(benchmark::State& state) {
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

static void BM_TBBSum(benchmark::State& state) {
    const int N = state.range(0);
    std::vector<double> data(N, 1.5);

    for (auto _ : state) {
        double sum = tbb::parallel_reduce(
            tbb::blocked_range<int>(0, N), 0.0,
            [&](const tbb::blocked_range<int>& r, double init) {
                for (int i = r.begin(); i < r.end(); ++i) {
                    init += data[i] * data[i];
                }
                return init;
            },
            std::plus<double>()
        );
        benchmark::DoNotOptimize(sum);
    }
}

BENCHMARK(BM_SerialSum)->Arg(100'000)->Arg(1'000'000);
BENCHMARK(BM_OMPSum)->Arg(100'000)->Arg(1'000'000);
BENCHMARK(BM_TBBSum)->Arg(100'000)->Arg(1'000'000);

int main(int argc, char** argv) {
    int omp_threads = omp_get_max_threads();
    int tbb_threads = static_cast<int>(tbb::info::default_concurrency());

    printf("OMP threads: %d\n", omp_threads);
    printf("TBB threads: %d\n", tbb_threads);

    // tbb warming up
    std::atomic<int> barrier{tbb_threads};
    tbb::parallel_for(0, tbb_threads, [&barrier] (int) {
        barrier--;
        while (barrier > 0) {
            std::this_thread::yield();
        }
    }, tbb::static_partitioner{});

    benchmark::Initialize(&argc, argv);
    if (benchmark::ReportUnrecognizedArguments(argc, argv)) return 1;
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
    return 0;
}
