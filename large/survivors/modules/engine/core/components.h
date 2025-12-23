#pragma once

#include <Eigen/Dense>

namespace core {
    struct Position {
        Eigen::Vector3f value;
    };

    struct Speed {
        float value;
    };

    struct GameSettings {
        std::string window_name;
        int initial_width;
        int initial_height;
        int window_width;
        int window_height;
    };

    struct Tag {
        std::string name;
    };

    struct DestroyAfterTime {
        float time;
    };

    struct DestroyAfterFrame {};
} // namespace core
