#pragma once

#include <Eigen/Dense>

#include <string>
#include <vector>

using namespace Eigen;

namespace core {
    // idea 1. use state variables with verbose name
    struct particle_q {
        Eigen::Vector2f value;
    };

    struct particle_qd {
        Eigen::Vector2f value;
    };

    // struct particle_q_new {
    //     Eigen::Vector2f value;
    // };

    // struct particle_qd_new {
    //     Eigen::Vector2f value;
    // };

    struct particle_f {
        Eigen::Vector2f value;
    };

    struct PARTICLE_FLAG_ACTIVE {};
    struct PARTICLE_FLAG_STATIC {};

    struct v_max {
        float value;
    };

    // enum struct ParticleState { PARTICLE_FLAG_ACTIVE = 0x00 };
    // struct particle_flag {
    //     unsigned int value;
    // };

    struct particle_radius {
        float value;
    };

    struct particle_mass {
        float value;
    };

    struct particle_inv_mass {
        float value;
    };

    struct gravity {
        Eigen::Vector2f value;
    };

    struct state {
        std::vector<Vector2f> particle_q;
        std::vector<Vector2f> partice_qd;
        std::vector<Vector2f> particle_f;
        std::vector<Vector3f> body_q;
        std::vector<Vector3f> body_qd;
        std::vector<Vector3f> body_f;

        inline int get_particle_count() { return particle_q.size(); }
        inline int get_body_count() { return body_q.size(); }
        inline void clear_particle_force() {
            for (auto &f: particle_f) {
                f.setZero();
            }
        }
        inline void clear_body_force() {
            for (auto &f: body_f) {
                f.setZero();
            }
        }
    };

    // idea 2. use state variables with short name
    // struct q {
    //     Eigen::Vector2f _;
    // };

    // struct qd {
    //     Eigen::Vector2f _;
    // };

    // struct f {
    //     Eigen::Vector2f _;
    // };

    // idea 3. use primitive variables
    // struct x {
    //     Eigen::Vector2f _;
    // };

    // struct v {
    //     Eigen::Vector2f _;
    // };

    // struct f {
    //     Eigen::Vector2f _;
    // };

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
