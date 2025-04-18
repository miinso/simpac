// pbd/types.hpp
#pragma once

#include <Eigen/Dense>
#include <flecs.h>

typedef double Real;

using Matrix3r = Eigen::Matrix<Real, 3, 3, Eigen::DontAlign>;
using Matrix4r = Eigen::Matrix<Real, 4, 4, Eigen::DontAlign>;
using Vector3r = Eigen::Matrix<Real, 3, 1, Eigen::DontAlign>;
using Quaternionr = Eigen::Quaternion<Real, Eigen::DontAlign>;

namespace phys {
    namespace pbd {
        // world quantities, unless otherwise mentioned

        // state (x, R, v, omega)
        struct Position         { Vector3r value = Vector3r::Zero(); }; //
        struct Orientation      { Quaternionr value = Quaternionr::Identity(); }; //
        struct LinearVelocity   { Vector3r value = Vector3r::Zero(); }; //
        struct AngularVelocity  { Vector3r value = Vector3r::Zero(); }; // body space

        // init state
        struct Position0         { Vector3r value = Vector3r::Zero(); };
        struct Orientation0      { Quaternionr value = Quaternionr::Identity(); };
        struct LinearVelocity0   { Vector3r value = Vector3r::Zero(); };
        struct AngularVelocity0  { Vector3r value = Vector3r::Zero(); }; // body space

        // prev state
        struct OldPosition         { Vector3r value = Vector3r::Zero(); };
        struct OldOrientation      { Quaternionr value = Quaternionr::Identity(); };
        struct OldLinearVelocity   { Vector3r value = Vector3r::Zero(); };
        struct OldAngularVelocity  { Vector3r value = Vector3r::Zero(); }; // body space

        struct LinearForce      { Vector3r value = Vector3r::Zero(); };
        struct AngularForce     { Vector3r value = Vector3r::Zero(); }; // torque

        struct LocalInertia { Matrix3r value = Matrix3r::Identity(); };
        struct LocalInverseInertia { Matrix3r value = Matrix3r::Identity(); };
        struct WorldInertia { Matrix3r value = Matrix3r::Identity(); };
        struct WorldInverseInertia { Matrix3r value = Matrix3r::Identity(); };
        
        // struct Scale            { Vector3r value = Vector3r::Ones(); };
        struct Mass             { Real value = 1.0f; };
        struct InverseMass { Real value = 1.0f; };

        struct Restitution { Real value = 0.5f; };
        struct DynamicFriction { Real value = 0.3f; };
        struct StaticFriction { Real value = 0.3f; };

        // flags (empty components for tagging)
        struct Static { };
        struct Dynamic { };
        struct Kinematic { };
        ////

        struct IsPinned { };

        struct Particle { };
        struct RigidBody { };

        static constexpr float _EPSILON = 1e-6f;

        // struct Constraint { };

        // struct DistanceConstraint {
        //     flecs::entity e1;
        //     flecs::entity e2;
        //     Real rest_distance;
        //     Real stiffness;
        //     Real lambda = 0;

        //     void solve(float dt) {
        //         auto x1 = e1.get_mut<Position>();
        //         auto x2 = e2.get_mut<Position>();

        //         auto w1 = e1.get<InverseMass>();
        //         auto w2 = e2.get<InverseMass>();

        //         auto w = w1->value + w2->value;
        //         if (w < _EPSILON)
        //             return;

        //         auto delta = x1->value - x2->value;
        //         auto distance = delta.norm();

        //         if (distance < _EPSILON)
        //             return;

        //         auto grad = delta / distance;

        //         auto C = distance - rest_distance;
        //         auto alpha = 1.0f / (stiffness * dt * dt);
        //         auto delta_lambda = -(C + alpha * lambda) / (w + alpha);
        //         // lambda = -s?
        //         lambda += delta_lambda;

        //         x1->value += w1->value * delta_lambda * grad;
        //         x2->value += w2->value * delta_lambda * -grad;
        //     }
        // };

        //
        struct Scene {
            Real timestep;
            int num_substeps;
            int solve_iter;
            Vector3r gravity;
            Real elapsed = 0;
        };

        enum class ConstraintType {
            DISTANCE,
            CONTACT,
            POSITIONAL,
            COUNT
        };

        enum class AxisType {
            POSITIVE_X,
            NEGATIVE_X,
            POSITIVE_Y,
            NEGATIVE_Y,
            POSITIVE_Z,
            NEGATIVE_Z
        };

        struct PositionalConstraint {
            Vector3r r1_lc; // application point for body1. local
            Vector3r r2_lc;
            Real compliance;
            Real lambda;
            Vector3r distance;
        };

        struct SphericalJointConstraint {
            Vector3r r1_lc;
            Vector3r r2_lc;
            Real lambda_pos;

            Real lambda_swing;
            Real swing_upper_limit;
            Real swing_lower_limit;
            AxisType e1_swing_axis;
            AxisType e2_swing_axis;

            Real lambda_twist;
            Real twist_upper_limit;
            Real twist_lower_limit;
            AxisType e1_twist_axis;
            AxisType e2_twist_axis;
        };

        struct CollisionConstraint {
            Vector3r r1_lc;
            Vector3r r2_lc;
            Vector3r normal; // world? local?
            Real lambda_t;
            Real lambda_n;
        };

        // constraints
        struct Constraint {
            ConstraintType type;
            flecs::entity e1;
            flecs::entity e2;

            union {
                PositionalConstraint positional_constraint;
                SphericalJointConstraint spherical_joint_constraint;
                CollisionConstraint collision_constraint;
            };
        };

    } // namespace pbd
} // namespace phys