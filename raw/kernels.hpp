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

        // calculate external force (grav, mouse interaction ...)

        inline void rb_integrate(Vector3r& x, Quaternionr& q, Vector3r& v, Vector3r& omega, const Matrix3r& I_body, const Matrix3r& I_body_inv, 
            const Vector3r& linear_force, const Vector3r& angular_force, const Real mass, const Real dt) {
            // ode linear part
            auto linear_acceleration = linear_force / mass;
            v += dt * linear_acceleration;
            x += v;

            // angular part
            auto angular_acceleration = I_body_inv * (angular_force - omega.cross(I_body * omega));
            omega += dt * angular_acceleration;

            // 1st taylor
            // q(t+dt) ~= q(t) + 1/2 * dt * (q(t) * omega) 

                // express the omega in pure quaternion form
                auto omega_q = Quaternionr(0, omega.x(), omega.y(), omega.z());
                // omega_q.coeffs() *= 0.5 * dt; // 0.5 for double-cover, dt bc it's unit speed integrated during the timestep
                
                // omega here is a body quantity, so right-multiply. R(q)
                // dq = 0.5 * q * omega_q
                auto q_dot = (q * omega_q);
                
                q.w() += 0.5 * dt * q_dot.w();
                q.vec() += 0.5 * dt * q_dot.vec();
                q.normalize();

            // or exponential map
                // axis = omega / omega.norm();
                // angle = omega.norm() * dt; unit speed multiplied with discrete time step
                // q_change = [cos(angle/2); sin(angle/2) * axis];
                // q = q * q_change; // right multiply bc omega is a body quantity
                // hamiltonian product between two unit quats give you another unit quat, so no normalize. (..or do we?)

        }

        inline void rb_update_velocity(Vector3r& v, Vector3r& omega, const Vector3r& x, const Vector3r& x_old, const Quaternionr& q, const Quaternionr& q_old, const Real dt) {
            v = (x - x_old) / dt;
            
            // again, omega is a body quantity, so change goes to the right side
            // q_new = q_old * q_change => inv(q_old) * q_new = q_change
            auto q_change = q_old.conjugate() * q;

            // inverse op
            omega = 2.0 * q_change.vec() / dt;

            // send to antipodal, since we want the shortest path
            if (q_change.w() < 0.0) {
                omega = -omega;
            }
        }

        inline void rb_reset(Vector3r& x, Quaternionr& q, Vector3r& v, Vector3r& omega, 
            const Vector3r& x0, const Quaternionr& q0, const Vector3r& v0, const Vector3r& omega0) {
            x = x0;
            q = q0;
            v = v0;
            omega = omega0;
        }

        inline void rb_clear_force(Vector3r& f, Vector3r& tau) {
            f.setZero();
            tau.setZero();
        }

        // inline void quat_rot_vec(Vector3r& v, const Quaternionr& q) {
        //     v = q * v * q.conjugate();
        // }

        
    } // namespace pbd
} // namespace phys