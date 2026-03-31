#include <benchmark/benchmark.h>

#include "../components.h"
#include "../physics/bridge.h"
#include "../physics/spring.h"
#include "../queries.h"
#include "../vars.h"

#include <Eigen/Sparse>
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

// inline solver for bench (same algorithm as main_implicit.cpp)
struct ImplicitEuler {
    Eigen::SparseMatrix<Real> A;
    Eigen::VectorXr b, x, x_prev;
    std::vector<Eigen::Triplet<Real>> triplets;
    Eigen::ConjugateGradient<Eigen::SparseMatrix<Real>> cg;
    bool exploded = false;

    void reset() { x.setZero(); x_prev.setZero(); exploded = false; }

    void step(const physics::Model& model, physics::State& state, Real dt,
              const Eigen::Vector3r& gravity, int max_iter, Real tolerance) {
        const int n = model.particle_count;
        if (n == 0) return;
        const int dof = n * 3;
        if (b.size() != dof) {
            b.resize(dof); x.resize(dof); x_prev.resize(dof); A.resize(dof, dof);
            x.setZero(); x_prev.setZero();
        }
        if (x.size() > 0 && (!x.allFinite() || !x_prev.allFinite())) exploded = true;
        if (exploded) reset();
        b.setZero(); triplets.clear();

        for (int i = 0; i < n; i++) {
            if (model.particle_inv_mass[i] <= Real(0)) continue;
            const Real mass = model.particle_mass[i];
            b.segment<3>(i*3) += mass * state.qd(i) + dt * mass * gravity;
            for (int d = 0; d < 3; d++) triplets.push_back({i*3+d, i*3+d, mass});
        }
        for (int s = 0; s < model.spring_count; s++) {
            const int i = model.spring_indices[s*2], j = model.spring_indices[s*2+1];
            physics::spring::Eval e;
            if (!physics::spring::eval(state.q(i), state.q(j), state.qd(i), state.qd(j),
                                       model.spring_rest_length[s], e)) continue;
            const bool fi = model.particle_inv_mass[i] > 0, fj = model.particle_inv_mass[j] > 0;
            const auto g = physics::spring::grad(model.spring_stiffness[s], model.spring_damping[s], e);
            if (fi) b.segment<3>(i*3) -= dt * g;
            if (fj) b.segment<3>(j*3) += dt * g;
            const auto H = physics::spring::hess(model.spring_stiffness[s], model.spring_rest_length[s], e);
            const Real h2 = dt * dt;
            for (int r = 0; r < 3; r++) for (int c = 0; c < 3; c++) {
                const Real val = h2 * H(r, c);
                if (fi) triplets.push_back({i*3+r, i*3+c, val});
                if (fj) triplets.push_back({j*3+r, j*3+c, val});
                if (fi && fj) { triplets.push_back({i*3+r, j*3+c, -val}); triplets.push_back({j*3+r, i*3+c, -val}); }
            }
        }
        A.setFromTriplets(triplets.begin(), triplets.end());
        cg.setMaxIterations(max_iter); cg.setTolerance(tolerance); cg.compute(A);
        x = cg.solveWithGuess(b, x_prev); x_prev = x;
        if (cg.info() != Eigen::Success || !x.allFinite()) { exploded = true; return; }
        for (int i = 0; i < n; i++) {
            if (model.particle_inv_mass[i] <= Real(0)) continue;
            state.qd(i) = x.segment<3>(i*3); state.q(i) += dt * state.qd(i);
        }
    }
};

struct BenchContext {
    flecs::world ecs;
    flecs::entity cg_max_iter;
    flecs::entity cg_tolerance;

    physics::Bridge bridge;
    physics::Model model;
    physics::State sim_state;
    ImplicitEuler solver;
    bool model_dirty = true;
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
    components::register_core_components(ctx.ecs);
    components::register_solver_component(ctx.ecs);

    queries::seed(ctx.ecs);
    props::seed(ctx.ecs);
    state::seed(ctx.ecs);

    ctx.cg_max_iter = ctx.ecs.entity("Config::Solver::cg_max_iter").set<int>(100).add<Configurable>();
    ctx.cg_tolerance = ctx.ecs.entity("Config::Solver::cg_tolerance").set<Real>(Real(1e-5f)).add<Configurable>();

    ctx.ecs.ensure<Solver>();

    // observers for model rebuild
    ctx.ecs.observer<Particle>()
        .event(flecs::OnAdd)
        .event(flecs::OnRemove)
        .run([&ctx](flecs::iter&) { ctx.model_dirty = true; });

    ctx.ecs.observer<Spring>()
        .event(flecs::OnSet)
        .event(flecs::OnAdd)
        .event(flecs::OnRemove)
        .each([&ctx](flecs::iter&, size_t, Spring&) { ctx.model_dirty = true; });

    // solver system using Model/State/Bridge
    ctx.ecs.system("Implicit Euler")
        .kind(flecs::PreUpdate)
        .immediate()
        .run([&ctx](flecs::iter& it) {
            auto world = it.world();

            if (ctx.model_dirty) {
                ctx.model = ctx.bridge.build(world);
                ctx.sim_state = ctx.model.state();
                ctx.solver.reset();
                ctx.model_dirty = false;
            }

            if (ctx.model.particle_count == 0) return;

            world.defer_suspend();
            ctx.bridge.gather(ctx.sim_state);

            const Real dt = props::dt.get<Real>();
            const Eigen::Vector3r gravity = props::gravity.get<vec3f>().map();
            const int max_iter = ctx.cg_max_iter.get<int>();
            const Real tolerance = ctx.cg_tolerance.get<Real>();

            ctx.solver.step(ctx.model, ctx.sim_state, dt, gravity, max_iter, tolerance);

            ctx.bridge.scatter(ctx.sim_state);
            world.defer_resume();
        });

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
