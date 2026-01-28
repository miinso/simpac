#include <benchmark/benchmark.h>
#include <flecs.h>

#include <random>
#include <vector>

struct Position {
    float x;
    float y;
    float z;
};

struct Spring {
    flecs::entity a;
    flecs::entity b;
};

struct SpringRef {
    flecs::ref<Position> a;
    flecs::ref<Position> b;

    SpringRef(flecs::ref<Position> a_, flecs::ref<Position> b_) : a(a_), b(b_) {}
};

static void seed_particles(flecs::world& ecs, int count, std::vector<flecs::entity>& out) {
    out.reserve((size_t)count);
    for (int i = 0; i < count; ++i) {
        out.push_back(ecs.entity().set<Position>({(float)i, (float)i * 0.5f, (float)i * 0.25f}));
    }
}

static void seed_springs(const std::vector<flecs::entity>& particles,
    int count,
    std::vector<Spring>& out) {
    std::mt19937 rng(123);
    std::uniform_int_distribution<int> pick(0, (int)particles.size() - 1);

    out.reserve((size_t)count);
    for (int i = 0; i < count; ++i) {
        int ia = pick(rng);
        int ib = pick(rng);
        out.push_back({particles[(size_t)ia], particles[(size_t)ib]});
    }
}

static void seed_refs(const std::vector<Spring>& springs, std::vector<SpringRef>& out) {
    out.reserve(springs.size());
    for (const auto& s : springs) {
        out.emplace_back(s.a.get_ref<Position>(), s.b.get_ref<Position>());
    }
}

static void BM_EntityGet(benchmark::State& state) {
    int particles = (int)state.range(0);
    int springs = (int)state.range(1);

    flecs::world ecs;
    ecs.component<Position>();

    std::vector<flecs::entity> entities;
    std::vector<Spring> edges;

    seed_particles(ecs, particles, entities);
    seed_springs(entities, springs, edges);

    for (auto _ : state) {
        float sum = 0.0f;
        for (const auto& s : edges) {
            const Position& a = s.a.get<Position>();
            const Position& b = s.b.get<Position>();
            sum += a.x + b.x;
        }
        benchmark::DoNotOptimize(sum);
    }

    state.SetItemsProcessed((int64_t)edges.size() * state.iterations());
}

static void BM_RefGet(benchmark::State& state) {
    int particles = (int)state.range(0);
    int springs = (int)state.range(1);

    flecs::world ecs;
    ecs.component<Position>();

    std::vector<flecs::entity> entities;
    std::vector<Spring> edges;
    std::vector<SpringRef> refs;

    seed_particles(ecs, particles, entities);
    seed_springs(entities, springs, edges);
    seed_refs(edges, refs);

    for (auto _ : state) {
        float sum = 0.0f;
        for (auto& s : refs) {
            auto* a = s.a.get();
            auto* b = s.b.get();
            sum += a->x + b->x;
        }
        benchmark::DoNotOptimize(sum);
    }

    state.SetItemsProcessed((int64_t)refs.size() * state.iterations());
}

BENCHMARK(BM_EntityGet)
    ->Args({4096, 2000000})
    ->Args({65536, 2000000})
    ->Args({262144, 2000000});

BENCHMARK(BM_RefGet)
    ->Args({4096, 2000000})
    ->Args({65536, 2000000})
    ->Args({262144, 2000000});

int main(int argc, char** argv) {
    benchmark::Initialize(&argc, argv);
    if (benchmark::ReportUnrecognizedArguments(argc, argv)) return 1;
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
    return 0;
}
