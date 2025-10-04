#include "raylib.h"

#ifdef EMSCRIPTEN
#include <emscripten/emscripten.h>
#include <emscripten/threading.h>
#endif

#include <Eigen/Dense>
#include <chrono>
#include <cpuinfo.h>
#include <info.hpp>
#include <iostream>
#include <sstream>
#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>


int screenWidth = 800;
int screenHeight = 450;
static std::vector<std::string> isa_list;
static std::string flags;

inline bool is_render_thread() {
#ifdef EMSCRIPTEN
    return emscripten_is_main_browser_thread();
#else
    return true;
#endif
}

void UpdateDrawFrame(void); // Update and Draw one frame

static double run_gemm(int N = 256) {
    using Clock = std::chrono::high_resolution_clock;
    Eigen::MatrixXd A = Eigen::MatrixXd::Random(N, N);
    Eigen::MatrixXd B = Eigen::MatrixXd::Random(N, N);
    Eigen::MatrixXd C = Eigen::MatrixXd::Zero(N, N);

    auto t0 = Clock::now();
#if 1
    tbb::parallel_for(tbb::blocked_range<size_t>(0, (size_t) N),
                      [&](const tbb::blocked_range<size_t> &r) {
                          for (size_t i = r.begin(); i != r.end(); ++i) {
                              C.row(i) = A.row(i) * B;
                          }
                      });
#else
    for (int i = 0; i < N; ++i) {
        C.row(i) = A.row(i) * B;
    }
#endif
    auto t1 = Clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
}

int main() {
    cpuinfo_initialize();
    isa_list = cpuinfo_utils::supported_isas();

    std::ostringstream oss;
    for (size_t i = 0; i < isa_list.size(); ++i) {
        if (i)
            oss << ',';
        oss << isa_list[i];
    }
    flags = oss.str();
    std::cout << flags << std::endl;

    InitWindow(screenWidth, screenHeight, "raylib [others] example - web basic window");

#if defined(EMSCRIPTEN)
    emscripten_set_main_loop(UpdateDrawFrame, 0, 1);
#else
    SetTargetFPS(60); // Set our game to run at 60 frames-per-second

    while (!WindowShouldClose()) // Detect window close button or ESC key
    {
        UpdateDrawFrame();
    }
#endif

    CloseWindow(); // Close window and OpenGL context


    cpuinfo_deinitialize();

    return 0;
}

void UpdateDrawFrame(void) {
    constexpr int GEMM_N = 512;
    double ms = run_gemm(GEMM_N);

    BeginDrawing();

    ClearBackground(RAYWHITE);

    DrawText("Congrats! You created your first window!", 190, 200, 20, LIGHTGRAY);
    DrawFPS(10, 10);
    
    DrawText(TextFormat("isa: %s", flags.c_str()), 10, 40, 10, DARKGRAY);
    DrawText(TextFormat("GEMM %dx%d: %.1f ms", GEMM_N, GEMM_N, ms), 10, 50, 10, DARKGRAY);

    EndDrawing();
}
