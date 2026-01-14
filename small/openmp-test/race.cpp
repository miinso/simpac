#include <cstdio>
#include <atomic>
#include <thread>
#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>
#include <tbb/task_arena.h>
#include <tbb/global_control.h>
#include <tbb/partitioner.h>

#ifdef __EMSCRIPTEN__
    #include "simpleomp.h"
#else
    #include <omp.h>
#endif

int main() {
    int omp_threads = omp_get_max_threads();
    int tbb_threads = static_cast<int>(tbb::info::default_concurrency());

    printf("OMP threads: %d\n", omp_threads);
    printf("TBB threads: %d\n", tbb_threads);

// #ifdef __EMSCRIPTEN__
    // Warm up TBB thread pool (a must for WASM, maybe for native?)
    // See: https://github.com/uxlfoundation/oneTBB/blob/master/WASM_Support.md
    std::atomic<int> barrier{tbb_threads};
    tbb::parallel_for(0, tbb_threads, [&barrier] (int) {
        barrier--;
        while (barrier > 0) {
            std::this_thread::yield();
        }
    }, tbb::static_partitioner{});
// #endif

    // Serial execution
    for (int i = 0; i < 8; i++) {
        printf("Serial: %d (thread %d)\n", i, omp_get_thread_num());
    }

    // OpenMP parallel execution
    printf("\n");
    #pragma omp parallel for
    for (int i = 0; i < 8; i++) {
        printf("OpenMP: %d (thread %d)\n", i, omp_get_thread_num());
    }

    // TBB parallel execution
    printf("\n");
    tbb::parallel_for(tbb::blocked_range<int>(0, 8), [](const tbb::blocked_range<int>& r) {
        for (int i = r.begin(); i != r.end(); ++i) {
            printf("TBB: %d (thread %d)\n", i, tbb::this_task_arena::current_thread_index());
        }
    });
}
