// pbd/types.hpp
#pragma once

#include <Eigen/Dense>
#include <flecs.h>
#include <vector>
#include <raylib.h>

typedef double Real;

// using Matrix3r = Eigen::Matrix<Real, 3, 3, Eigen::DontAlign>;
// using Matrix4r = Eigen::Matrix<Real, 4, 4, Eigen::DontAlign>;
// using Vector3r = Eigen::Matrix<Real, 3, 1, Eigen::DontAlign>;
// using Quaternionr = Eigen::Quaternion<Real, Eigen::DontAlign>;

using Vector2r = Eigen::Matrix<Real, 2, 1, Eigen::DontAlign>;
using Vector3r = Eigen::Matrix<Real, 3, 1, Eigen::DontAlign>;
using Vector4r = Eigen::Matrix<Real, 4, 1, Eigen::DontAlign>;
using Vector5r = Eigen::Matrix<Real, 5, 1, Eigen::DontAlign>;
using Vector6r = Eigen::Matrix<Real, 6, 1, Eigen::DontAlign>;
using Matrix2r = Eigen::Matrix<Real, 2, 2, Eigen::DontAlign>;
using Matrix3r = Eigen::Matrix<Real, 3, 3, Eigen::DontAlign>;
using Matrix4r = Eigen::Matrix<Real, 4, 4, Eigen::DontAlign>;
using Vector2i = Eigen::Matrix<int, 2, 1, Eigen::DontAlign>;
using Vector3i = Eigen::Matrix<int, 3, 1, Eigen::DontAlign>;
using Vector4i = Eigen::Matrix<int, 4, 1, Eigen::DontAlign>;
using AlignedBox2r = Eigen::AlignedBox<Real, 2>;
using AlignedBox3r = Eigen::AlignedBox<Real, 3>;
using AngleAxisr = Eigen::AngleAxis<Real>;
using Quaternionr = Eigen::Quaternion<Real, Eigen::DontAlign>;

namespace prefabs {
struct Particle {};
struct RigidBody {};
} // namespace prefabs

// RenderMesh component hold raylib::model object
struct RenderMesh
{
    Model model;

    // void set_transform(Matrix4r &m)
    // {
    // 	model.transform = MatrixIdentity();
    // };
};

typedef enum {
    BOX_COLLIDER,
    SPHERE_COLLIDER,
    CAPSULE_COLLIDER,
    MESH_COLLIDER,
    SDF_COLLIDER
} PhysicsMeshType;

struct PhysicsMesh // collider?
{
	Model model;
	Matrix3r inertia; // body space

	Matrix3r compute_inertia()
	{
		assert(model.meshes[0].triangleCount > 0);

        // do the volume integration thing?
        // extend the raylib for physics stuff
	};
	Vector3r compute_inertia_diagonalized(Matrix3r &R, Vector3r &t);
};

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
        struct DeactivationTime { Real value = 0.0; };
        struct IsActive { };
        struct BoundingSphere { Real value = 1; }; // radius

        struct Mesh0 {
            Mesh m;
        };

        // struct Colliders { std::vector<Collider> value; };

        typedef struct {
            Vector3r position;
            Vector3r force;
            bool is_world_space; // default local
        } Physics_Force;

        struct Forces {
            std::vector<Physics_Force> a;
        };

        // flags (empty components for tagging)
        struct Static { };
        struct Dynamic { };
        struct Kinematic { };
        ////

        struct IsPinned { };

        struct Particle { };
        struct RigidBody { };

        static constexpr float _EPSILON = 1e-6f;

        // Scene configuration and state
        struct Scene {
            // Simulation parameters
            Real dt;                    // timestep per simulation step
            int num_substeps;           // PBD substeps
            int solve_iter;             // constraint solver iterations
            Vector3r gravity;           // gravity vector
            
            // Runtime state
            Real wall_time = 0;         // real elapsed time (wall-clock)
            Real sim_time = 0;          // accumulated simulation time
            int frame_count = 0;        // number of simulation steps executed
        };

        typedef enum {
            POSITIONAL_CONSTRAINT,
            COLLISION_CONSTRAINT,
            MUTUAL_ORIENTATION_CONSTRAINT,
            HINGE_JOINT_CONSTRAINT,
            SPHERICAL_JOINT_CONSTRAINT
        } Constraint_Type;

        typedef enum AxisType {
            POSITIVE_X,
            NEGATIVE_X,
            POSITIVE_Y,
            NEGATIVE_Y,
            POSITIVE_Z,
            NEGATIVE_Z
        } AxisType;

        // typedef double r64;

        // typedef union
        // {
        //     struct
        //     {
        //         r64 x, y, z;
        //     };
        //     struct
        //     {
        //         r64 r, g, b;
        //     };
        // } vec3;

        typedef struct {
            Vector3r r1_lc; // application point for body1. defined in b1's local frame
            Vector3r r2_lc;
            Real compliance;
            Real lambda;
            Real distance;
        } PositionalConstraint;
        
        typedef struct {
            Real compliance;
            Real lambda;
        } MutualOrientationConstraint;

        typedef struct {
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
        } SphericalJointConstraint;

        typedef struct {
            Vector3r r1_lc;
            Vector3r r2_lc;
            Vector3r normal; // world? local?
            Real lambda_t;
            Real lambda_n;
        } CollisionConstraint;

        // constraints
        typedef struct {
            Constraint_Type type;
            flecs::entity e1;
            flecs::entity e2;

            PositionalConstraint positional_constraint;
            // SphericalJointConstraint spherical_joint_constraint;
            CollisionConstraint collision_constraint;
            MutualOrientationConstraint mutual_orientation_constraint;    
            // union {
                // PositionalConstraint positional_constraint;
                // SphericalJointConstraint spherical_joint_constraint;
                // CollisionConstraint collision_constraint;
            // };

        } Constraint;

        

    } // namespace pbd
} // namespace phys