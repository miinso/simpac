#pragma once

#include <Eigen/Dense>

#include <string>
#include <vector>

using namespace Eigen;


namespace core {
    constexpr float PI = 3.14159265359f;

    struct rotation {
        float s, c;
    };

    struct transform {
        Vector2f p;
        rotation q;
    };

    struct bound {
        Vector2f l;
        Vector2f u;

        // TODO: add the usual static methods
    };

    // idea 1. use state variables with verbose name
    struct particle_q {
        Vector2f value;
    };

    struct particle_qd {
        Vector2f value;
    };

    // struct particle_q_new {
    //     Eigen::Vector2f value;
    // };

    // struct particle_qd_new {
    //     Eigen::Vector2f value;
    // };

    struct particle_f {
        Vector2f value;
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
        Vector2f value;
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

    // TODO: decide how to treat flags
    // enum body_type { STATIC_BODY = 0, KINEMATIC_BODY = 1, DYNAMIC_BODY = 2 };
    // or
    // struct BODY_TYPE_STATIC {};
    // struct BODY_TYPE_KINEMATIC {};
    // struct BODY_TYPE_DYNAMIC {};

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
    inline rotation make_rotation(float angle) { return {sin(angle), cos(angle)}; }

    static inline Matrix2f get_rotation_from_xf(const transform &xf) {
        return Matrix2f{xf.q.c, -xf.q.s, xf.q.s, xf.q.c};
    }
    // local to world
    static inline Vector2f transform_point(const Vector2f &p, const transform &xf) {
        const auto R = get_rotation_from_xf(xf);
        return R * p + xf.p;
    }

    static inline Vector2f transform_vector(const Vector2f &p, const transform &xf) {
        const auto R = get_rotation_from_xf(xf);
        return R * p;
    }
} // namespace core
