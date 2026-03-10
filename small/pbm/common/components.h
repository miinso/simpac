#pragma once

#include "Eigen/Dense"

constexpr int CANVAS_WIDTH = 800;
constexpr int CANVAS_HEIGHT = 600;
constexpr float COLLISION_EPSILON = 1e-6f;

struct Position { Eigen::Vector3f value; };
struct Velocity { Eigen::Vector3f value; };
struct Acceleration { Eigen::Vector3f value; };

struct SceneBase {
    Eigen::Vector3f gravity;
    float air_resistance;
    float simulation_time_step;
    float accumulated_time;
    double frame_time;
    double sim_time;
};
