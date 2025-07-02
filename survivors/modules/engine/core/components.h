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
        std::string windowName;
        int initialWidth;
        int initialHeight;
        int windowWidth;
        int windowHeight;
    };

    struct Tag {
        std::string name;
    };

    struct DestroyAfterTime {
        float time;
    };

    struct DestroyAfterFrame {};
} // namespace core
