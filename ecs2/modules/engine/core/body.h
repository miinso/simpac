#pragma once

#include <Eigen/Dense>
#include <cstdint>
#include "components.h"
#include "geometry.h"

// using namespace Eigen;
using namespace core;

namespace body {
    enum body_type { STATIC, KINEMATIC, DYNAMIC };

    struct body_desc {
        uint8_t type;
        Eigen::Vector2f position;
        float angle;
        Eigen::Vector2f velocity_linear;
        float velocity_angular;
    };

    struct body {
        // body_type type;
        uint8_t type;
        Eigen::Vector2f origin; // initial position not accounting it's child shapes. used as a
                                // reference point when computing total mass, I
        Eigen::Vector2f position; // center of mass in world coords. updated when shapes are added
        Eigen::Vector2f position_delta; // amount of position change in this simulation (sub)step
        Eigen::Vector2f position0; // initial world position. ignores shape add event
        rotation rot; // [cos(); sin()], a 2-vec
        rotation rot0; // initial rotation.
        Eigen::Vector2f
                local_center; // translational offset wrt `origin` to account the child shapes' com

        Eigen::Vector2f velocity_linear; // world
        float velocity_angular; // body
        Eigen::Vector2f velocity_linear0;
        float velocity_angular0;

        Eigen::Vector2f force; // world?
        float torque; // body?

        std::vector<geometry::shape> shape_list;
        // std::vector<contact::contact> contact_list;
        // std::vector<joint::joint> joint_list;

        float mass;
        float inv_mass;
        float I; // about the center of mass
        float inv_I;
    };

    static inline bool should_two_bodies_collide(const body &b1, const body &b2, float dt) {
        // TODO: collision culling for jointed bodies
        return false;
    }


} // namespace body

#include <flecs.h>

namespace body2 {
    // flags
    struct BODY_TYPE_STATIC {};
    struct BODY_TYPE_KINEMATIC {};
    struct BODY_TYPE_DYNAMIC {};

    struct Body {}; // an emtpy tag
    struct Origin {
        Eigen::Vector2f value;
    };
    struct Position {
        Eigen::Vector2f value;
    };
    struct PositionDelta {
        Eigen::Vector2f value;
    };
    struct Position0 {
        Eigen::Vector2f value;
    };
    struct Rotation {
        rotation value;
    };
    struct Rotation0 {
        rotation value;
    };
    struct LocalCenter {
        Eigen::Vector2f value;
    };

    struct VelocityLinear {
        Eigen::Vector2f value;
    };
    struct VelocityAngular {
        float value;
    };
    struct VelocityLinear0 {
        Eigen::Vector2f value;
    };
    struct VelocityAngular0 {
        float value;
    };

    struct Force {
        Eigen::Vector2f value;
    };
    struct Torque {
        float value;
    };

    // std::vector<int> shape_list; // what to do with this??
    // add as entity("shape").child_of(body) then

    // 1. get all shape entity
    // world.system<geometry::shape>("get all shape components").term_at(0).desc();

    // 2. observe `shape` add event
    // world.observer

    // 3. Iterate all children for a parent
    // parent.children([](flecs::entity child) {
    //      if (child.has<Shape>()) {}
    // });

    struct Mass {
        float value;
    };
    struct InvMass {
        float value;
    };
    struct Inertia {
        float value;
    };
    struct InvInertia {
        float value;
    };

} // namespace body2
