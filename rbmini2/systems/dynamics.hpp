// systems/dynamics.hpp
#pragma once
#include "dynamics/forces.hpp"
#include "dynamics/integration.hpp"
#include "dynamics/types.hpp"
#include "util/logging.hpp"
#include <flecs.h>

using namespace phys::components;

namespace phys {
    namespace systems {
        void reset_forces(flecs::iter& it, size_t, LinearForce& force, AngularForce& torque) {
            force.value.setZero();
            torque.value.setZero();
        }

        void apply_gravity(flecs::iter& it, size_t, LinearForce& force, const Mass& mass) {
            force.value += dynamics::compute_gravity_force(mass.value, constants::GRAVITY);
        }

        void update_inertia(flecs::iter& it,
                            size_t,
                            WorldInertia& world_inertia,
                            WorldInverseInertia& world_inv_inertia,
                            const LocalInertia& local_inertia,
                            const Orientation& orientation) {
            Matrix3r R = orientation.value.matrix();
            // I = R * I_local * R^T
            world_inertia.value = R * local_inertia.value * R.transpose();
            world_inv_inertia.value = world_inertia.value.inverse();
        }

        void integrate_rigid_bodies(
            flecs::iter& it,
            size_t i,
            Position& position,
            Orientation& orientation,
            LinearVelocity& linear_velocity,
            AngularVelocity& angular_velocity,
            const LinearForce& force,
            const AngularForce& torque,
            const Mass& mass,
            const WorldInverseInertia& inverse_inertia) {
            const Real dt = it.delta_time();

            if (!it.entity(i).has<components::Static>()) {
                dynamics::integrate_linear_motion(
                    position.value, linear_velocity.value, force.value, mass.value, dt);

                // position.value = Vector3r{0.0f, 1.5f, 0.0f};
                log_info("%f %f %f", position.value[0], position.value[1], position.value[2]);

                dynamics::integrate_angular_motion(
                    orientation.value,
                    angular_velocity.value,
                    torque.value,
                    inverse_inertia.value,
                    dt);
            }

            log_info("integrate_rigid_bodies::%s", it.entity(i).view().name());
        }

        void register_dynamics_systems(flecs::world& ecs) {
            ecs.system<LinearForce, AngularForce>().kind(flecs::PreUpdate).each(reset_forces);

            ecs.system<LinearForce, const Mass>().kind(flecs::OnUpdate).each(apply_gravity);

            ecs.system<WorldInertia, WorldInverseInertia, const LocalInertia, const Orientation>()
                .kind(flecs::OnUpdate)
                .each(update_inertia);

            ecs.system<Position,
                       Orientation,
                       LinearVelocity,
                       AngularVelocity,
                       const LinearForce,
                       const AngularForce,
                       const Mass,
                       const WorldInverseInertia>()
                .kind(flecs::OnUpdate)
                .each(integrate_rigid_bodies);
        }
    } // namespace systems
} // namespace phys