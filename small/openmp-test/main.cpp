#include <cstdio>

#ifdef __EMSCRIPTEN__
    #include "simpleomp.h" // for browser
#else
    #include <omp.h>
#endif

int main() {
    printf("Max threads: %d\n", omp_get_max_threads());

#ifdef _OPENMP
    printf("_OPENMP defined: %d\n", _OPENMP);
#else
    printf("_OPENMP not defined\n");
#endif

    // Without OpenMP
    for (int i = 0; i < 8; i++) {
        printf("Serial: %d (thread %d)\n", i, omp_get_thread_num());
    }

    // With OpenMP
    #pragma omp parallel for
    for (int i = 0; i < 8; i++) {
        printf("Parallel: %d (thread %d)\n", i, omp_get_thread_num());
    }
}
