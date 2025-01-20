// core/components.hpp
#pragma once
#include "core/types.hpp"

namespace phys {
    namespace components {
        // transform
        struct Position {
            Vector3r value = Vector3r::Zero();
        };

        struct Orientation {
            Quaternionr value = Quaternionr::Identity();
        };

        struct Scale {
            Vector3r value = Vector3r::Ones();
        };

        // dynamics
        struct LinearVelocity {
            Vector3r value = Vector3r::Zero();
        };

        struct AngularVelocity {
            Vector3r value = Vector3r::Zero();
        };

        struct LinearForce {
            Vector3r value = Vector3r::Zero();
        };

        struct AngularForce {
            Vector3r value = Vector3r::Zero();
        };

        struct Mass {
            Real value = 1.0f;
            static Mass create(Real m) {
                return {m};
            }
        };

        struct InverseMass {
            Real value = 1.0f;
            static InverseMass from_mass(Real m) {
                return {m != 0.0f ? 1.0f / m : 0.0f};
            }
        };

        struct LocalInertia {
            Matrix3r value = Matrix3r::Identity();
        };

        struct LocalInverseInertia {
            Matrix3r value = Matrix3r::Identity();
        };

        struct WorldInertia {
            Matrix3r value = Matrix3r::Identity();
        };

        struct WorldInverseInertia {
            Matrix3r value = Matrix3r::Identity();
        };

        struct Restitution {
            Real value = 0.5f;
        };

        struct Friction {
            Real value = 0.3f;
        };

        // flags (empty components for tagging)
        struct Static { };
        struct Dynamic { };
        struct Kinematic { };
        // struct Sleeping {};
        // struct Trigger {};

        struct DebugDraw {
            bool show_aabb = false;
            bool show_wireframe = false;
            bool show_contacts = false;
            bool show_constraints = false;
        };
    } // namespace components
} // namespace phys