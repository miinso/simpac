#pragma once

#include <Eigen/Dense>
#include <cstdint>
#include "components.h"
#include "geometry.h"

using namespace Eigen;
using namespace core;

namespace body {
    enum body_type { STATIC, KINEMATIC, DYNAMIC };

    struct body_desc {
        uint8_t type;
        Vector2f position;
        float angle;
        Vector2f velocity_linear;
        float velocity_angular;
    };

    struct body {
        // body_type type;
        uint8_t type;
        Vector2f origin; // initial position not accounting it's child shapes. used as a reference
                         // point when computing total mass, I
        Vector2f position; // center of mass in world coords. updated when shapes are added
        Vector2f position_delta; // amount of position change in this simulation (sub)step
        Vector2f position0; // initial world position. ignores shape add event
        rotation rot; // [cos(); sin()], a 2-vec
        rotation rot0; // initial rotation.
        Vector2f local_center; // translational offset wrt `origin` to account the child shapes' com

        Vector2f velocity_linear; // world
        float velocity_angular; // body
        Vector2f velocity_linear0;
        float velocity_angular0;

        Vector2f force; // world?
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
        Vector2f value;
    };
    struct Position {
        Vector2f value;
    };
    struct PositionDelta {
        Vector2f value;
    };
    struct Position0 {
        Vector2f value;
    };
    struct Rotation {
        rotation value;
    };
    struct Rotation0 {
        rotation value;
    };
    struct LocalCenter {
        Vector2f value;
    };

    struct VelocityLinear {
        Vector2f value;
    };
    struct VelocityAngular {
        float value;
    };
    struct VelocityLinear0 {
        Vector2f value;
    };
    struct VelocityAngular0 {
        float value;
    };

    struct Force {
        Vector2f value;
    };
    struct Torque {
        float value;
    };

    std::vector<int> shape_list; // what to do with this??
    // add as entity("shape").child_of(body) then
    
    void create_system() {
        auto world = flecs::world();
        // 1. get all shape entity
        world.system<geometry::shape>("get all shape components").term_at(0).desc();

        // 2. observe `shape` add event
        // world.observer

        // Iterate all children for a parent
        // parent.children([](flecs::entity child) {
        //  if (child.has<Shape>()) {}
        // });
    }

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
