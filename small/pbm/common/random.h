#pragma once

#include "Eigen/Dense"
#include <random>

inline float uniformRandom(float min = -1.0f, float max = 1.0f) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(min, max);
    return dis(gen);
}

inline Eigen::Vector3f randomVector() {
    return Eigen::Vector3f(
        uniformRandom(), uniformRandom(), uniformRandom()
    ).normalized();
}
