// dynamics/integration.hpp
#pragma once
#include "dynamics/types.hpp"

namespace phys {
    namespace dynamics {
        inline void integrate_linear_motion(Vector3r& position,
                                            Vector3r& velocity,
                                            const Vector3r& force,
                                            const Real mass,
                                            const Real dt) {
            // v(t+dt) = v(t) + (f/m)dt
            velocity += (force / mass) * dt;
            // x(t+dt) = x(t) + v(t+dt)dt
            position += velocity * dt;
        }

        inline void integrate_angular_motion(Quaternionr& orientation,
                                             Vector3r& angular_velocity,
                                             const Vector3r& torque,
                                             const Matrix3r& inverse_inertia,
                                             const Real dt) {
            // angular acceleration: alpha = Iinv(tau - omega.cross(I * omega))
            Matrix3r inertia = inverse_inertia.inverse();
            Vector3r alpha =
                inverse_inertia * (torque - angular_velocity.cross(inertia * angular_velocity));

            // angular velocity: omega(t+dt) = omega(t) + alpha * dt
            angular_velocity += alpha * dt;

            // new orientation: q(t+dt) = q(t) + 1/2 * dt * (omega * q(t))
            Quaternionr omega_quat(
                0, angular_velocity.x(), angular_velocity.y(), angular_velocity.z());
            orientation.coeffs() += 0.5f * dt * (orientation * omega_quat).coeffs();
            orientation.normalize();
        }
    } // namespace dynamics
} // namespace phys