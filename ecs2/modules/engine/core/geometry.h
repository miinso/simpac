#pragma once

// shape and geometry
#include <Eigen/Dense>
#include <cstdint>
#include "components.h"


using namespace Eigen;
using namespace core;

namespace geometry {

    struct mass_info {
        float mass;
        Vector2f center;
        float I;
    };

    enum shape_type { CIRCLE, CAPSULE };

    struct circle {
        Vector2f p; // in world coords? no, it's wrt body origin.
        float radius;
        Vector2f pad;
    };

    struct capsule {
        Vector2f p1;
        Vector2f p2;
        float radius;
    };

    struct shape {
        // object obj; = entity
        // int body_index = parent.entity_id
        uint8_t type;
        float density;
        float friction;
        float restitution;
        uint8_t collision_filter;
        bound aabb;
        bound aabb_fat;

        shape() :
            type(CIRCLE), density(1.0f), friction(1.0f), restitution(0.0f), collision_filter(0) {
            circle.p = Vector2f::Zero();
            circle.radius = 1.0f;
        }

        shape(const shape &s) :
            type(s.type), density(s.density), friction(s.friction), restitution(s.restitution),
            collision_filter(0) {
            switch (s.type) {
                case CIRCLE:
                    circle = s.circle;
                case CAPSULE:
                    capsule = s.capsule;
            }
        }


        union {
            circle circle;
            capsule capsule;
        };
    };

    static mass_info compute_mass_circle(const circle &shape, float density) {
        mass_info info;
        auto r_squared = shape.radius * shape.radius;
        info.mass = density * r_squared * PI;
        info.center = shape.p;

        // inertia about the local origin
        info.I = info.mass * (0.5f * r_squared + shape.p.dot(shape.p));

        return info;
    }

    static mass_info compute_mass_capsule(const capsule &shape, float density) {
        mass_info info;
        auto r = shape.radius;
        auto r_squared = r * r;
        auto p1 = shape.p1;
        auto p2 = shape.p2;
        float length = (p2 - p1).norm();
        float ll = length * length;

        info.mass = density * (PI * r_squared + 2 * r * length);
        info.center = (p1 + p2) * 0.5f;

        float I_circle = 0.5f * (r_squared + ll);
        float I_box = (4.0f * r_squared + ll) / 12.0f;
        info.I = info.mass * (I_circle + I_box);

        return info;
    }

    static inline bound compute_aabb_circle(const circle &shape, transform xf) {
        auto p = transform_point(shape.p, xf);
        float r = shape.radius;

        bound aabb;
        aabb.l = p.array() - r;
        aabb.u = p.array() + r; // p + Vector2f::Constant(r);

        return aabb;
    }

    static inline bound compute_aabb_capsule(const capsule &shape, transform xf) {
        auto p1 = transform_point(shape.p1, xf);
        auto p2 = transform_point(shape.p2, xf);
        float r = shape.radius;

        bound aabb;
        aabb.l = p1.cwiseMin(p2).array() - r;
        aabb.u = p1.cwiseMax(p2).array() + r;

        return aabb;
    }

    static mass_info compute_mass(const shape &s) {
        switch (s.type) {
            case CIRCLE:
                return compute_mass_circle(s.circle, s.density);
            case CAPSULE:
                return compute_mass_capsule(s.capsule, s.density);
            default:
                assert(false);
                // return {0, {0, 0}, 0};
        }
    }

    static bound compute_aabb(const shape &s, transform xf) {
        switch (s.type) {
            case CIRCLE:
                return compute_aabb_circle(s.circle, xf);
            case CAPSULE:
                return compute_aabb_capsule(s.capsule, xf);
            default:
                assert(false);
                // return {Vector2f::Zero(), Vector2f::Zero()};
        }
    }

    inline shape create_circle_shape() {
        shape s;
        s.type = CIRCLE;
        s.circle.p;

        return s;
    }
} // namespace geometry
