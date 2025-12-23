// pbd/kernels.hpp
#pragma once
#include "types.hpp"

namespace phys {
    namespace pbd {
        // particle things
        inline void particle_clear_acceleration(Vector3r& acceleration, Vector3r& gravity) {
            acceleration = gravity;
        }

        inline void particle_clear_constraint_lambda(DistanceConstraint& c) {
            c.lambda = 0;
        }

        inline void particle_integrate(Vector3r& x,
                                       Vector3r& x_old,
                                       Vector3r& v,
                                       const Real mass,
                                       const Vector3r& a,
                                       const Real dt) {
            x_old = x;

            v += dt * a;
            x += dt * v;
        }

        inline void particle_solve_constraint(DistanceConstraint& c, const Real dt) {
            c.solve(dt);
        };

        inline void particle_solve_distance_constraint(
            Vector3r& x1,
            Vector3r& x2,
            Real& lambda,
            const Real w1,
            const Real w2,
            const Real stiffness,
            const Real rest_distance,
            const Real dt) {

            // JMinvJ', norm(J) is 1 so ignore
            auto w = w1 + w2;

            if (w < _EPSILON) {
                return;
            }

            auto delta = x1 - x2;
            auto distance = delta.norm();

            if (distance < _EPSILON) {
                return;
            }

            auto gradC_x1 = delta / distance;
            auto gradC_x2 = -gradC_x1;

            auto C = distance - rest_distance;

            auto alpha = 1.0f / (stiffness * dt * dt);
            auto delta_lambda = -(C + alpha * lambda) / (w + alpha);

            // cumulate lambda
            lambda += delta_lambda;

            // apply mass-weighted corrections
            x1 += w1 * delta_lambda * gradC_x1;
            x2 += w2 * delta_lambda * gradC_x2;
        }

        inline void particle_ground_collision(
            Vector3r& x, const Vector3r& x_old, const Real dt, const Real friction = 0.8f) {
            if (x.y() < 0) {
                x.y() = 0;

                auto displacement = x - x_old;

                auto friction_factor = std::min(1.0, dt * friction);
                x.x() = x_old.x() + displacement.x() * (1 - friction_factor);
                x.z() = x_old.z() + displacement.z() * (1 - friction_factor);
            }
        }

        inline void particle_update_velocity(
            Vector3r& v, const Vector3r& x, const Vector3r& x_old, const Real dt) {
            v = (x - x_old) / dt;
        }
    } // namespace pbd
} // namespace phys