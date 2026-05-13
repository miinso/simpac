// mass-spring implicit euler benchmark

#include <benchmark/benchmark.h>

#include "../bridge/bridge.h"
#include "../math/spring.h"
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

// =========================================================================
// cg solver state
// =========================================================================

namespace cg {
inline Eigen::SparseMatrix<Real> A;
inline Eigen::VectorXr b, x, x_prev;
inline std::vector<Eigen::Triplet<Real>> triplets;
inline Eigen::ConjugateGradient<Eigen::SparseMatrix<Real>> solver;
inline bool exploded = false;
} // namespace cg

// =========================================================================
// flow
// =========================================================================

namespace flow {

inline void prepare(flecs::iter&) {
    const int dof = sim::model.particle_count * 3;
    if (cg::b.size() != dof) {
        cg::b.resize(dof); cg::x.resize(dof); cg::x_prev.resize(dof);
        cg::A.resize(dof, dof);
        cg::x.setZero(); cg::x_prev.setZero();
    }
    if (cg::x.size() > 0 && (!cg::x.allFinite() || !cg::x_prev.allFinite()))
        cg::exploded = true;
    if (cg::exploded) {
        cg::x.setZero(); cg::x_prev.setZero(); cg::exploded = false;
    }
    cg::b.setZero();
    cg::triplets.clear();
}

inline void assemble_momentum(flecs::iter&) {
    for (int i : sim::model.free_particles)
        cg::b.segment<3>(i * 3) += sim::model.particle_mass[i] * sim::state_0.qd(i);
}

inline void assemble_external_force(flecs::iter& it) {
    const Real dt = it.delta_time();
    for (int i : sim::model.free_particles)
        cg::b.segment<3>(i * 3) += dt * sim::model.particle_mass[i] * sim::gravity;
}

inline void assemble_spring_force(flecs::iter& it) {
    const Real dt = it.delta_time();
    for (int s = 0; s < sim::model.spring_count; s++) {
        const int i = sim::model.spring_indices[s * 2];
        const int j = sim::model.spring_indices[s * 2 + 1];
        physics::spring::Eval e;
        if (!physics::spring::eval(sim::state_0.q(i), sim::state_0.q(j),
                                   sim::state_0.qd(i), sim::state_0.qd(j),
                                   sim::model.spring_rest_length[s], e)) continue;
        const auto g = physics::spring::grad(sim::model.spring_stiffness[s],
                                             sim::model.spring_damping[s], e);
        if (!(sim::model.particle_flags[i] & physics::PARTICLE_FLAG_PINNED)) cg::b.segment<3>(i * 3) -= dt * g;
        if (!(sim::model.particle_flags[j] & physics::PARTICLE_FLAG_PINNED)) cg::b.segment<3>(j * 3) += dt * g;
    }
}

inline void assemble_inertia(flecs::iter&) {
    for (int i : sim::model.free_particles) {
        const Real mass = sim::model.particle_mass[i];
        for (int d = 0; d < 3; d++)
            cg::triplets.push_back({i * 3 + d, i * 3 + d, mass});
    }
}

inline void assemble_spring_stiffness(flecs::iter& it) {
    const Real h2 = it.delta_time() * it.delta_time();
    for (int s = 0; s < sim::model.spring_count; s++) {
        const int i = sim::model.spring_indices[s * 2];
        const int j = sim::model.spring_indices[s * 2 + 1];
        physics::spring::Eval e;
        if (!physics::spring::eval(sim::state_0.q(i), sim::state_0.q(j),
                                   sim::state_0.qd(i), sim::state_0.qd(j),
                                   sim::model.spring_rest_length[s], e)) continue;
        const Eigen::Matrix3r H = physics::spring::hess(
            sim::model.spring_stiffness[s], sim::model.spring_rest_length[s], e);
        const bool i_free = !(sim::model.particle_flags[i] & physics::PARTICLE_FLAG_PINNED);
        const bool j_free = !(sim::model.particle_flags[j] & physics::PARTICLE_FLAG_PINNED);
        for (int r = 0; r < 3; r++) for (int c = 0; c < 3; c++) {
            const Real val = h2 * H(r, c);
            if (i_free) cg::triplets.push_back({i*3+r, i*3+c, val});
            if (j_free) cg::triplets.push_back({j*3+r, j*3+c, val});
            if (i_free && j_free) {
                cg::triplets.push_back({i*3+r, j*3+c, -val});
                cg::triplets.push_back({j*3+r, i*3+c, -val});
            }
        }
    }
}

inline void solve(flecs::iter&) {
    cg::A.setFromTriplets(cg::triplets.begin(), cg::triplets.end());
    cg::solver.setMaxIterations(100);
    cg::solver.setTolerance(Real(1e-5f));
    cg::solver.compute(cg::A);
    cg::x = cg::solver.solveWithGuess(cg::b, cg::x_prev);
    cg::x_prev = cg::x;
    if (cg::solver.info() != Eigen::Success || !cg::x.allFinite())
        cg::exploded = true;
}

inline void update_velocity(flecs::iter&) {
    if (cg::exploded) return;
    for (int i : sim::model.free_particles)
        sim::state_0.qd(i) = cg::x.segment<3>(i * 3);
}

inline void integrate_position(flecs::iter& it) {
    if (cg::exploded) return;
    const Real dt = it.delta_time();
    for (int i : sim::model.free_particles)
        sim::state_0.q(i) += dt * sim::state_0.qd(i);
}

} // namespace flow

// =========================================================================
// module: bench solver (mass-spring implicit euler, no stats)
// =========================================================================

namespace cloth {

struct bench_solver {
    bench_solver(flecs::world& ecs) {
        flecs::system prepare, assemble_momentum, assemble_external_force;
        flecs::system assemble_spring_force, assemble_inertia, assemble_spring_stiffness;
        flecs::system solve, update_velocity, integrate_position;

        auto solver_parent = ecs.entity("Implicit Euler (bench)");
        ecs.scope(solver_parent, [&] {
            prepare = ecs.system("Prepare").kind(0).run(flow::prepare);
            assemble_momentum = ecs.system("Assemble Momentum").kind(0).run(flow::assemble_momentum);
            assemble_external_force = ecs.system("Assemble External Force").kind(0).run(flow::assemble_external_force);
            assemble_spring_force = ecs.system("Assemble Spring Force").kind(0).run(flow::assemble_spring_force);
            assemble_inertia = ecs.system("Assemble Inertia").kind(0).run(flow::assemble_inertia);
            assemble_spring_stiffness = ecs.system("Assemble Spring Stiffness").kind(0).run(flow::assemble_spring_stiffness);
            solve = ecs.system("Solve").kind(0).run(flow::solve);
            update_velocity = ecs.system("Update Velocity").kind(0).run(flow::update_velocity);
            integrate_position = ecs.system("Integrate Position").kind(0).run(flow::integrate_position);
        });

        ecs.system("Step")
            .kind(sim::Simulate)
            .run([=](flecs::iter&) {
                const Real dt = props::dt.get<Real>();
                sim::gravity = props::gravity.get<vec3r>().map();
                prepare.run(dt);
                assemble_momentum.run(dt);
                assemble_external_force.run(dt);
                assemble_spring_force.run(dt);
                assemble_inertia.run(dt);
                assemble_spring_stiffness.run(dt);
                solve.run(dt);
                update_velocity.run(dt);
                integrate_position.run(dt);
sim::bridge.scatter(sim::state_0);
            });
    }
};

} // namespace cloth

// =========================================================================
// benchmark harness
// =========================================================================

namespace {

std::string script_template_path() {
    namespace fs = std::filesystem;
    fs::path path = "small/cloth/bench/bench.flecs";
    if (const char* workspace = std::getenv("BUILD_WORKSPACE_DIRECTORY")) {
        if (*workspace) path = fs::path(workspace) / path;
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

bool load_scene_script(flecs::world& ecs, int size, std::string& error) {
    std::string script_text = read_text_file(script_template_path());
    if (script_text.empty()) {
        error = "failed to read small/cloth/bench/bench.flecs";
        return false;
    }
    replace_all(script_text, "__WIDTH__", std::to_string(size));
    replace_all(script_text, "__HEIGHT__", std::to_string(size));

    auto script = ecs.script("BenchScene").code(script_text.c_str()).run();
    if (!script) { error = "script entity creation failed"; return false; }
    if (const EcsScript* data = script.try_get<EcsScript>(); data && data->error) {
        error = data->error; return false;
    }
    return true;
}

static void BM_ClothImplicit(benchmark::State& state) {
    const int size = (int)state.range(0);

    flecs::world ecs;
    ecs.import<cloth::particle>();
    ecs.import<cloth::element>();
    ecs.import<cloth::bridge>();
    ecs.import<cloth::bench_solver>();

    std::string error;
    if (!load_scene_script(ecs, size, error)) {
        state.SkipWithError(error.c_str());
        return;
    }

    const double particle_count = (double)queries::num_particles();
    const double spring_count = (double)queries::num_springs();

    for (int i = 0; i < 10; ++i)
        ecs.progress((float)props::dt.get<Real>());

    for (auto _ : state) {
        ecs.progress((float)props::dt.get<Real>());
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
    for (int i = 1; i < argc; ++i)
        if (std::strncmp(argv[i], prefix, len) == 0) return true;
    return false;
}

std::string default_result_path() {
    namespace fs = std::filesystem;
    fs::path out_dir = "small/cloth/bench/results";
    if (const char* workspace = std::getenv("BUILD_WORKSPACE_DIRECTORY")) {
        if (*workspace) out_dir = fs::path(workspace) / out_dir;
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
    for (int i = 0; i < argc; ++i) owned_args.push_back(argv[i]);

    if (!has_flag(argc, argv, "--benchmark_out=")) {
        const std::string out = default_result_path();
        owned_args.push_back(std::string("--benchmark_out=") + out);
        if (!has_flag(argc, argv, "--benchmark_out_format="))
            owned_args.push_back("--benchmark_out_format=json");
        std::printf("[bench] benchmark_out %s\n", out.c_str());
    }

    std::vector<char*> arg_ptrs;
    arg_ptrs.reserve(owned_args.size());
    for (std::string& s : owned_args) arg_ptrs.push_back((char*)s.c_str());

    int bench_argc = (int)arg_ptrs.size();
    benchmark::Initialize(&bench_argc, arg_ptrs.data());
    if (benchmark::ReportUnrecognizedArguments(bench_argc, arg_ptrs.data())) return 1;
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
    return 0;
}
