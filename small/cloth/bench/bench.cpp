#include <benchmark/benchmark.h>

#include "../components.h"
#include "../queries.h"
#include "../systems.h"
#include "../vars.h"
#include "bench.h"

#include <chrono>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace {

struct BenchContext {
    // one ecs world per benchmark case
    flecs::world ecs;
    flecs::entity cg_max_iter;
    flecs::entity cg_tolerance;
};

std::string script_template_path() {
    namespace fs = std::filesystem;
    // prefer workspace root when running under bazel
    fs::path path = "small/cloth/bench/bench.flecs";
    if (const char* workspace = std::getenv("BUILD_WORKSPACE_DIRECTORY")) {
        if (*workspace) {
            path = fs::path(workspace) / path;
        }
    }
    return path.string();
}

std::string read_text_file(const std::string& path) {
    std::ifstream in(path, std::ios::in | std::ios::binary);
    if (!in) return {};
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

void replace_all(std::string& s, const std::string& from, const std::string& to) {
    size_t pos = 0;
    while ((pos = s.find(from, pos)) != std::string::npos) {
        s.replace(pos, from.size(), to);
        pos += to.size();
    }
}

bool load_scene_script(BenchContext& ctx, int size, std::string& error) {
    // fill template placeholders for current size
    std::string script_text = read_text_file(script_template_path());
    if (script_text.empty()) {
        error = "failed to read small/cloth/bench/bench.flecs";
        return false;
    }
    replace_all(script_text, "__WIDTH__", std::to_string(size));
    replace_all(script_text, "__HEIGHT__", std::to_string(size));

    auto script = ctx.ecs.script("BenchScene").code(script_text.c_str()).run();
    if (!script) {
        error = "script entity creation failed";
        return false;
    }

    if (const EcsScript* data = script.try_get<EcsScript>(); data && data->error) {
        error = data->error;
        return false;
    }

    return true;
}

bool setup_context(BenchContext& ctx, int size, std::string& error) {
    // register ecs types then install runtime systems
    components::register_core_components(ctx.ecs);
    components::register_solver_component(ctx.ecs);

    queries::seed(ctx.ecs);
    props::seed(ctx.ecs);
    state::seed(ctx.ecs);

    ctx.cg_max_iter = ctx.ecs.entity("Config::Solver::cg_max_iter").set<int>(100).add<Configurable>();
    ctx.cg_tolerance = ctx.ecs.entity("Config::Solver::cg_tolerance").set<Real>(Real(1e-5f)).add<Configurable>();

    ctx.ecs.ensure<Solver>();

    systems::install_scene_systems(ctx.ecs);
    systems::bench::install(ctx.ecs, ctx.cg_max_iter, ctx.cg_tolerance);

    if (!load_scene_script(ctx, size, error)) {
        return false;
    }

    return true;
}

static void BM_ClothImplicit(benchmark::State& state) {
    const int size = (int)state.range(0);

    BenchContext ctx;
    std::string error;
    if (!setup_context(ctx, size, error)) {
        state.SkipWithError(error.c_str());
        return;
    }

    const double particle_count = (double)queries::num_particles();
    const double spring_count = (double)queries::num_springs();

    // warmup before measured iterations
    for (int i = 0; i < 10; ++i) {
        ctx.ecs.progress((float)props::dt.get<Real>());
    }

    for (auto _ : state) {
        ctx.ecs.progress((float)props::dt.get<Real>());
        benchmark::ClobberMemory();
    }

    state.counters["particles"] = particle_count;
    state.counters["springs"] = spring_count;
    state.counters["dof"] = particle_count * 3.0;
    state.SetItemsProcessed((int64_t)state.iterations() * (int64_t)particle_count);
}

BENCHMARK(BM_ClothImplicit)
    ->Arg(50)
    ->Arg(100)
    ->Arg(200)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime();

bool has_flag(int argc, char** argv, const char* prefix) {
    const size_t len = std::strlen(prefix);
    for (int i = 1; i < argc; ++i) {
        if (std::strncmp(argv[i], prefix, len) == 0) {
            return true;
        }
    }
    return false;
}

std::string default_result_path() {
    namespace fs = std::filesystem;
    fs::path out_dir = "small/cloth/bench/results";
    if (const char* workspace = std::getenv("BUILD_WORKSPACE_DIRECTORY")) {
        if (*workspace) {
            out_dir = fs::path(workspace) / out_dir;
        }
    }
    fs::create_directories(out_dir);

    const auto now = std::chrono::system_clock::now();
    const std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm = {};
#if defined(_WIN32)
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif

    std::ostringstream stamp;
    stamp << std::put_time(&tm, "%Y%m%d_%H%M%S");
    return (out_dir / ("bench_" + stamp.str() + ".json")).string();
}

} // namespace

int main(int argc, char** argv) {
    std::vector<std::string> owned_args;
    owned_args.reserve((size_t)argc + 2);
    for (int i = 0; i < argc; ++i) {
        owned_args.push_back(argv[i]);
    }

    if (!has_flag(argc, argv, "--benchmark_out=")) {
        // auto emit json when caller did not provide output flags
        const std::string out = default_result_path();
        owned_args.push_back(std::string("--benchmark_out=") + out);
        if (!has_flag(argc, argv, "--benchmark_out_format=")) {
            owned_args.push_back("--benchmark_out_format=json");
        }
        std::printf("[bench] benchmark_out %s\n", out.c_str());
    }

    std::vector<char*> arg_ptrs;
    arg_ptrs.reserve(owned_args.size());
    for (std::string& s : owned_args) {
        arg_ptrs.push_back((char*)s.c_str());
    }

    int bench_argc = (int)arg_ptrs.size();
    benchmark::Initialize(&bench_argc, arg_ptrs.data());
    if (benchmark::ReportUnrecognizedArguments(bench_argc, arg_ptrs.data())) return 1;
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
    return 0;
}
