#pragma once

#include <Eigen/Dense>

#include <cmath>
#include <string>


namespace core {


    struct rotation {
        float s, c;
    };

    struct transform {
        Eigen::Vector2f p;
        rotation q;
    };

    struct bound {
        Eigen::Vector2f l;
        Eigen::Vector2f u;

        // TODO: add the usual static methods
    };

    // idea 1. use state variables with verbose name
    struct particle_q {
        Eigen::Vector2f value;
    };

    struct particle_qd {
        Eigen::Vector2f value;
    };

    struct particle_radius {
        float value{1.0f};
    };

    struct Position {
        Eigen::Vector2f value;
    };

    struct Settings {
        std::string window_name;
        int initial_width;
        int initial_height;
        int window_width;
        int window_height;
    };
} // namespace core

namespace core {
    [[nodiscard]] inline rotation make_rotation(float angle) noexcept {
        return {std::sin(angle), std::cos(angle)};
    }

    [[nodiscard]] inline Eigen::Matrix2f get_rotation_from_xf(const transform &xf) {
        Eigen::Matrix2f R;
        R << xf.q.c, -xf.q.s, xf.q.s, xf.q.c;
        return R;
    }

    [[nodiscard]] inline Eigen::Vector2f transform_point(const Eigen::Vector2f &p, const transform &xf) {
        const auto R = get_rotation_from_xf(xf);
        return R * p + xf.p;
    }

    [[nodiscard]] inline Eigen::Vector2f transform_vector(const Eigen::Vector2f &p, const transform &xf) {
        const auto R = get_rotation_from_xf(xf);
        return R * p;
    }
} // namespace core
