#pragma once
#include <cstdio>
#include <chrono>

#define GREEN "\033[32m"
#define RESET "\033[0m"

using Clock = std::chrono::high_resolution_clock;

struct Timer {
    Clock::time_point start, stop;
};

inline void startTime(Timer* timer) {
    timer->start = Clock::now();
}

inline void stopTime(Timer* timer) {
    timer->stop = Clock::now();
}

inline void printElapsedTime(Timer timer, const char* label, const char* color = nullptr) {
    double ms = std::chrono::duration<double, std::milli>(timer.stop - timer.start).count();
    if (color)
        printf("%s%-20s %10.3f ms%s\n", color, label, ms, RESET);
    else
        printf("%-20s %10.3f ms\n", label, ms);
}
