#pragma once

#define EIGEN_DONT_VECTORIZE
#include <Eigen/Dense>

#include "graphics.h"

using Vector3f = Eigen::Matrix<float, 3, 1, Eigen::DontAlign>;

struct Position { Eigen::Vector3f value; };
struct Velocity { Eigen::Vector3f value; };
struct Acceleration { Eigen::Vector3f value; };
struct OldPosition { Eigen::Vector3f value; };
struct Mass { float value; };
struct InverseMass { float value; };

struct IsPinned { };

struct Particle {};

struct Scene {
    float timestep;
    int num_substeps;
    int solve_iter;
    Vector3f gravity;
    float elapsed = 0;
};