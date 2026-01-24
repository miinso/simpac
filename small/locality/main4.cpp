#include <benchmark/benchmark.h>
#include <flecs.h>

#include <cstring>
#include <vector>

#include "components.h"

struct Particle {};
struct Red {};
struct Blue {};
struct Yellow {};
struct Green {};

static void seed_particles(flecs::world& ecs, int total, int tables) {
    if (tables < 1) tables = 1;
    if (tables > 4) tables = 4;

    for (int i = 0; i < total; ++i) {
        auto e = ecs.entity()
            .add<Particle>()
            .set<Position>({(Real)i, 0.0f, 0.0f})
            .set<Velocity>({0.0f, (Real)i, 0.0f});
        if (tables > 1) {
            switch (i % tables) {
                case 0: e.add<Red>(); break;
                case 1: e.add<Blue>(); break;
                case 2: e.add<Yellow>(); break;
                case 3: e.add<Green>(); break;
                default: break;
            }
        }
    }
}

static size_t count_particles(flecs::query<const Position, const Velocity>& q) {
    size_t total = 0;
    q.run([&](flecs::iter& it) {
        while (it.next()) {
            total += (size_t)it.count();
        }
    });
    return total;
}

static void BM_TableBulkMemcpy(benchmark::State& state) {
    int total = (int)state.range(0);
    int tables = (int)state.range(1);

    flecs::world ecs;
    ecs.component<Position>();
    ecs.component<Velocity>();
    ecs.component<Particle>();
    ecs.component<Red>();
    ecs.component<Blue>();
    ecs.component<Yellow>();
    ecs.component<Green>();

    seed_particles(ecs, total, tables);

    auto q = ecs.query_builder<const Position, const Velocity>()
        .with<Particle>()
        .build();

    size_t total_count = count_particles(q);
    std::vector<Real> pos_out(total_count * 3);
    std::vector<Real> vel_out(total_count * 3);
    size_t bytes_per_iter = total_count * 3 * sizeof(Real) * 2;

    for (auto _ : state) {
        size_t offset = 0;
        q.run([&](flecs::iter& it) {
            while (it.next()) {
                auto pos = it.field<const Position>(0);
                auto vel = it.field<const Velocity>(1);
                int n = it.count();
                if (n == 0) continue;
                std::memcpy(pos_out.data() + offset * 3, pos[0].data(),
                    (size_t)n * 3 * sizeof(Real));
                std::memcpy(vel_out.data() + offset * 3, vel[0].data(),
                    (size_t)n * 3 * sizeof(Real));
                offset += (size_t)n;
            }
        });
        benchmark::DoNotOptimize(pos_out.data());
        benchmark::DoNotOptimize(vel_out.data());
        benchmark::ClobberMemory();
    }

    state.SetBytesProcessed((int64_t)bytes_per_iter * state.iterations());
}

static void BM_TableElementCopy(benchmark::State& state) {
    int total = (int)state.range(0);
    int tables = (int)state.range(1);

    flecs::world ecs;
    ecs.component<Position>();
    ecs.component<Velocity>();
    ecs.component<Particle>();
    ecs.component<Red>();
    ecs.component<Blue>();
    ecs.component<Yellow>();
    ecs.component<Green>();

    seed_particles(ecs, total, tables);

    auto q = ecs.query_builder<const Position, const Velocity>()
        .with<Particle>()
        .build();

    size_t total_count = count_particles(q);
    std::vector<Real> pos_out(total_count * 3);
    std::vector<Real> vel_out(total_count * 3);
    size_t bytes_per_iter = total_count * 3 * sizeof(Real) * 2;

    for (auto _ : state) {
        size_t offset = 0;
        q.run([&](flecs::iter& it) {
            while (it.next()) {
                auto pos = it.field<const Position>(0);
                auto vel = it.field<const Velocity>(1);
                int n = it.count();
                if (n == 0) continue;
                Real* pos_dst = pos_out.data() + offset * 3;
                Real* vel_dst = vel_out.data() + offset * 3;
                for (int i = 0; i < n; ++i) {
                    size_t base = (size_t)i * 3;
                    pos_dst[base + 0] = pos[i].x;
                    pos_dst[base + 1] = pos[i].y;
                    pos_dst[base + 2] = pos[i].z;
                    vel_dst[base + 0] = vel[i].x;
                    vel_dst[base + 1] = vel[i].y;
                    vel_dst[base + 2] = vel[i].z;
                }
                offset += (size_t)n;
            }
        });
        benchmark::DoNotOptimize(pos_out.data());
        benchmark::DoNotOptimize(vel_out.data());
        benchmark::ClobberMemory();
    }

    state.SetBytesProcessed((int64_t)bytes_per_iter * state.iterations());
}

BENCHMARK(BM_TableBulkMemcpy)
    ->Args({400000, 1})
    ->Args({400000, 2})
    ->Args({400000, 3})
    ->Args({400000, 4});
BENCHMARK(BM_TableElementCopy)
    ->Args({400000, 1})
    ->Args({400000, 2})
    ->Args({400000, 3})
    ->Args({400000, 4});

int main(int argc, char** argv) {
    benchmark::Initialize(&argc, argv);
    if (benchmark::ReportUnrecognizedArguments(argc, argv)) return 1;
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
    return 0;
}

// "400k entities!
// INFO: Running command line: bazel-bin/small/locality/locality4.exe
// 2026-01-24T18:48:42+09:00
// Running C:\users\user\_bazel_user\vj36eseu\execroot\_main\bazel-out\x64_windows-opt\bin\small\locality\locality4.exe
// Run on (16 X 3194 MHz CPU s)
// CPU Caches:
//   L1 Data 32 KiB (x8)
//   L1 Instruction 32 KiB (x8)
//   L2 Unified 512 KiB (x8)
//   L3 Unified 16384 KiB (x1)
// ---------------------------------------------------------------------------------------
// Benchmark                             Time             CPU   Iterations UserCounters...
// ---------------------------------------------------------------------------------------
// BM_TableBulkMemcpy/400000/1      247308 ns       249051 ns         2635 bytes_per_second=35.899Gi/s
// BM_TableBulkMemcpy/400000/2      267459 ns       266781 ns         2987 bytes_per_second=33.5132Gi/s
// BM_TableBulkMemcpy/400000/3      246740 ns       245857 ns         2987 bytes_per_second=36.3654Gi/s
// BM_TableBulkMemcpy/400000/4      525168 ns       531250 ns         1000 bytes_per_second=16.8295Gi/s
// BM_TableElementCopy/400000/1     726191 ns       732422 ns          896 bytes_per_second=12.207Gi/s
// BM_TableElementCopy/400000/2     643986 ns       645229 ns          896 bytes_per_second=13.8566Gi/s
// BM_TableElementCopy/400000/3     647026 ns       655692 ns         1120 bytes_per_second=13.6355Gi/s
// BM_TableElementCopy/400000/4     613016 ns       613839 ns         1120 bytes_per_second=14.5652Gi/s

// "1M entities!""
// INFO: Running command line: bazel-bin/small/locality/locality4.exe
// 2026-01-24T20:35:40+09:00
// Running C:\users\user\_bazel_user\vj36eseu\execroot\_main\bazel-out\x64_windows-opt\bin\small\locality\locality4.exe
// Run on (16 X 3194 MHz CPU s)
// CPU Caches:
//   L1 Data 32 KiB (x8)
//   L1 Instruction 32 KiB (x8)
//   L2 Unified 512 KiB (x8)
//   L3 Unified 16384 KiB (x1)
// ----------------------------------------------------------------------------------------
// Benchmark                              Time             CPU   Iterations UserCounters...
// ----------------------------------------------------------------------------------------
// BM_TableBulkMemcpy/1000000/1     1552423 ns      1562500 ns          560 bytes_per_second=14.3051Gi/s
// BM_TableBulkMemcpy/1000000/2     1336382 ns      1339286 ns          560 bytes_per_second=16.6893Gi/s
// BM_TableBulkMemcpy/1000000/3     1309878 ns      1311384 ns          560 bytes_per_second=17.0444Gi/s
// BM_TableBulkMemcpy/1000000/4     1256212 ns      1255580 ns          560 bytes_per_second=17.8019Gi/s
// BM_TableElementCopy/1000000/1    2294329 ns      2299331 ns          299 bytes_per_second=9.72098Gi/s
// BM_TableElementCopy/1000000/2    2322381 ns      2294922 ns          320 bytes_per_second=9.73965Gi/s
// BM_TableElementCopy/1000000/3    2283479 ns      2299331 ns          299 bytes_per_second=9.72098Gi/s
// BM_TableElementCopy/1000000/4    2353224 ns      2351589 ns          299 bytes_per_second=9.50495Gi/s

// more stats maybe
// bazel-bin\small\locality\locality4.exe --benchmark_min_time=0.5s --benchmark_repetitions=5 --benchmark_report_aggregates_only=true