// pbd/types.hpp
#pragma once
// #include "core/types.hpp"

#include <Eigen/Dense>
#include <flecs.h>
#include <iostream>

typedef double Real;
using Vector3r = Eigen::Matrix<Real, 3, 1, Eigen::DontAlign>;

namespace phys {
    namespace pbd {

        struct Position {
            Vector3r value;
        };
        struct Velocity {
            Vector3r value;
        };
        struct Acceleration {
            Vector3r value;
        };
        struct OldPosition {
            Vector3r value;
        };
        struct Mass {
            Real value;
        };
        struct InverseMass {
            Real value;
        };

        struct IsPinned { };

        struct Particle { };

        static constexpr float _EPSILON = 1e-6f;

        struct Constraint { };

        struct DistanceConstraint {
            flecs::entity e1;
            flecs::entity e2;
            Real rest_distance;
            Real stiffness;
            Real lambda = 0;

            void solve(float dt) {
                // Flecs 4.1.1+: get_mut<T>() and get<T>() return references, not pointers
                auto& x1 = e1.get_mut<Position>();
                auto& x2 = e2.get_mut<Position>();

                const auto& w1 = e1.get<InverseMass>();
                const auto& w2 = e2.get<InverseMass>();

                auto w = w1.value + w2.value;
                if (w < _EPSILON)
                    return;

                auto delta = x1.value - x2.value;
                auto distance = delta.norm();

                if (distance < _EPSILON)
                    return;

                auto grad = delta / distance;

                auto C = distance - rest_distance;
                auto alpha = 1.0f / (stiffness * dt * dt);
                auto delta_lambda = -(C + alpha * lambda) / (w + alpha);
                // lambda = -s?
                lambda += delta_lambda;

                x1.value += w1.value * delta_lambda * grad;
                x2.value += w2.value * delta_lambda * -grad;
            }
        };

        //
        // Scene configuration and state
        struct Scene {
            // Simulation parameters
            float dt;                   // timestep per simulation step
            int num_substeps;           // PBD substeps
            int solve_iter;             // constraint solver iterations
            Vector3f gravity;           // gravity vector
            
            // Runtime state
            float wall_time = 0;        // real elapsed time (wall-clock)
            float sim_time = 0;         // accumulated simulation time
            int frame_count = 0;        // number of simulation steps executed
        };

    } // namespace pbd
} // namespace phys