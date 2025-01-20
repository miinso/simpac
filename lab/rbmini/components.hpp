// components.hpp
#pragma once
#include <Eigen/Dense>

#include "graphics.h"
#include "IndexedFaceMesh.h"
#include "ParticleData.h"
// 

struct Geometry {
    Utilities::IndexedFaceMesh mesh;
    PBD::VertexData verts0; // local, constant
    PBD::VertexData verts; // world, keep updated every frame
    rl::Mesh renderable; // raylib mesh
};

//
// components
struct Position { Eigen::Vector3f value; };
struct Orientation { Eigen::Quaternionf value; };
struct Rotation { Eigen::Matrix3f value; };
struct LinearVelocity { Eigen::Vector3f value; };
struct AngularVelocity { Eigen::Vector3f value; };
struct Acceleration { Eigen::Vector3f value; };
struct LinearForce { Eigen::Vector3f value; };
struct AngularForce { Eigen::Vector3f value; };
struct OldPosition { Eigen::Vector3f value; };
struct OldOrientation { Eigen::Quaternionf value; };
struct Mass { float value; };
struct InverseMass { float value; };
struct Density { float value; };
struct LocalInertia { Eigen::Matrix3f value; };
struct LocalInverseInertia { Eigen::Matrix3f value; };
struct WorldInertia { Eigen::Matrix3f value; };
struct WorldInverseInertia { Eigen::Matrix3f value; };
struct Restitution { float value; };
struct Friction { float value; };

struct ConstraintForce { Eigen::Vector3f value; };     // fc
struct ConstraintTorque { Eigen::Vector3f value; };    // tauc

// extra
struct IsPinned { };

// rb specifics
struct RigidBody { }; // Tag
//

struct Contact {
    flecs::entity e1;
    flecs::entity e2;
    Vector3r x_world;
    Vector3r closest_point_world;
    Vector3r normal_world;
    Real dist;
    Real restitution;
    Real friction;

    Vector3r t; // first tangent direction
    Vector3r b; // second tangent direction (binormal)
    
    // jacobian blocks [normal; tangent1; tangent2] × [linear|angular]
    Eigen::Matrix<Real, 3, 6> J0;    // for body 1
    Eigen::Matrix<Real, 3, 6> J1;    // for body 2
    Eigen::Matrix<Real, 3, 6> J0_Minv;  // J0 * M^-1
    Eigen::Matrix<Real, 3, 6> J1_Minv;  // J1 * M^-1
    Vector3r lambda;              // contact forces (normal, tangent1, tangent2)
    Vector3r prev_lambda;

    // compute orthonormal contact frame
    void compute_contact_frame() {
        // find first tangent direction
        if (std::abs(normal_world.x()) < 0.5) {
            t = normal_world.cross(Vector3r(1, 0, 0));
        } else {
            t = normal_world.cross(Vector3r(0, 1, 0));
        }
        t.normalize();

        // compute binormal to complete orthonormal basis
        b = normal_world.cross(t);
        b.normalize();
    }

    // compute constraint jacobians
    void compute_jacobian() {
        J0.setZero();
        J1.setZero();
        J0_Minv.setZero();
        J1_Minv.setZero();
        lambda.setZero();

        // compute jacobian for first body if not fixed
        if (!e1.has<IsPinned>()) {
            auto pos1 = e1.get<Position>();
            auto mass1 = e1.get<Mass>();
            auto inertia1 = e1.get<WorldInverseInertia>();
            
            Vector3r r1 = x_world - pos1->value;

            // normal constraint row
            J0.block<1,3>(0,0) = normal_world.transpose();
            J0.block<1,3>(0,3) = r1.cross(normal_world).transpose();

            // first tangent direction
            J0.block<1,3>(1,0) = t.transpose();
            J0.block<1,3>(1,3) = r1.cross(t).transpose();

            // second tangent direction
            J0.block<1,3>(2,0) = b.transpose();
            J0.block<1,3>(2,3) = r1.cross(b).transpose();

            // compute J * M^-1 block
            J0_Minv.block<3,3>(0,0) = (1.0/mass1->value) * J0.block<3,3>(0,0);
            J0_Minv.block<3,3>(0,3) = J0.block<3,3>(0,3) * inertia1->value;
        }

        // compute jacobian for second body if not fixed
        if (!e2.has<IsPinned>()) {
            auto pos2 = e2.get<Position>();
            auto mass2 = e2.get<Mass>();
            auto inertia2 = e2.get<WorldInverseInertia>();
            
            Vector3r r2 = x_world - pos2->value;

            // Note: negative since constraint is: n^T (v2 - v1) >= 0
            J1.block<1,3>(0,0) = -normal_world.transpose();
            J1.block<1,3>(0,3) = -r2.cross(normal_world).transpose();

            J1.block<1,3>(1,0) = -t.transpose();
            J1.block<1,3>(1,3) = -r2.cross(t).transpose();

            J1.block<1,3>(2,0) = -b.transpose();
            J1.block<1,3>(2,3) = -r2.cross(b).transpose();

            // compute J * M^-1 block
            J1_Minv.block<3,3>(0,0) = (1.0/mass2->value) * J1.block<3,3>(0,0);
            J1_Minv.block<3,3>(0,3) = J1.block<3,3>(0,3) * inertia2->value;
        }
    }
};