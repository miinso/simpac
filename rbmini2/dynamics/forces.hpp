// dynamics/forces.hpp
#pragma once
#include "dynamics/types.hpp"

namespace phys {
    namespace dynamics {
        inline Vector3r compute_gravity_force(const Real mass, const Vector3r& gravity) {
            return mass * gravity;
        }

        inline Vector3r compute_drag_force(const Vector3r& velocity,
                                           const Real drag_coefficient,
                                           const Real fluid_density = 1.225f) {
            Real speed = velocity.norm();
            if (speed < constants::EPSILON)
                return Vector3r::Zero();

            Vector3r drag_direction = -velocity.normalized();
            Real drag_magnitude = 0.5f * fluid_density * speed * speed * drag_coefficient;
            return drag_direction * drag_magnitude;
        }

        inline Vector3r compute_spring_force(
            const Vector3r& pos1,
            const Vector3r& pos2,
            const Real rest_length,
            const Real stiffness,
            const Real damping,
            const Vector3r& vel1,
            const Vector3r& vel2) {

            // note it's x2 - x1, not x1 - x2
            Vector3r delta = pos2 - pos1;
            Real distance = delta.norm();

            Vector3r spring_force =
                stiffness * (distance - rest_length) / distance * delta.normalized();

            Vector3r damping_force = damping * (vel2 - vel1);

            return spring_force + damping_force;
        }
    } // namespace dynamics
} // namespace phys