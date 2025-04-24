// pbd/kernels.hpp
#pragma once
#include "constraints.h"
#include "types.h"
#include <broad.h>
#include <collider.h>
#include <vector>

namespace phys {
namespace pbd {

// particle thing
namespace particle {
inline void particle_solve_distance_constraint (Vector3r& x1,
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

    auto delta    = x1 - x2;
    auto distance = delta.norm ();

    if (distance < _EPSILON) {
        return;
    }

    auto gradC_x1 = delta / distance;
    auto gradC_x2 = -gradC_x1;

    auto C = distance - rest_distance;

    auto alpha        = 1.0f / (stiffness * dt * dt);
    auto delta_lambda = -(C + alpha * lambda) / (w + alpha);

    // cumulate lambda
    lambda += delta_lambda;

    // apply mass-weighted corrections
    x1 += w1 * delta_lambda * gradC_x1;
    x2 += w2 * delta_lambda * gradC_x2;
}

inline void particle_ground_collision (Vector3r& x,
const Vector3r& x_old,
const Real dt,
const Real friction = 0.8f) {
    if (x.y () < 0) {
        x.y () = 0;

        auto displacement = x - x_old;

        auto friction_factor = std::min (1.0, dt * friction);
        x.x () = x_old.x () + displacement.x () * (1 - friction_factor);
        x.z () = x_old.z () + displacement.z () * (1 - friction_factor);
    }
}

inline void
particle_update_velocity (Vector3r& v, const Vector3r& x, const Vector3r& x_old, const Real dt) {
    v = (x - x_old) / dt;
}
} // namespace particle

// rigidbody thing
namespace rigidbody {

#define ENABLE_SIMULATION_ISLANDS
#define LINEAR_SLEEPING_THRESHOLD 0.10
#define ANGULAR_SLEEPING_THRESHOLD 0.10
#define DEACTIVATION_TIME_TO_BE_INACTIVE 1.0

// inline void calculate_external_force (grav, mouse interaction ...)

inline void rb_integrate (Vector3r& x,
Vector3r& x_old,
Quaternionr& q,
Quaternionr& q_old,
Vector3r& v,
Vector3r& omega,
const Matrix3r& I_body,
const Matrix3r& I_body_inv,
const Vector3r& linear_force,
const Vector3r& angular_force,
const Real mass,
const Real dt) {
    x_old = x;
    q_old = q;

    // ode linear part
    auto linear_acceleration = linear_force / mass;
    v += dt * linear_acceleration;
    x += v * dt;

    // angular part
    auto angular_acceleration =
    I_body_inv * (angular_force - omega.cross (I_body * omega));
    omega += dt * angular_acceleration;

    // 1st taylor
    // q(t+dt) ~= q(t) + 1/2 * dt * (q(t) * omega)
    // express the omega in pure quaternion form
    auto omega_q = Quaternionr (0, omega.x (), omega.y (), omega.z ());

    // omega here is a body quantity, so right-multiply. ~= R(omega_q)*q or
    // L(q)*omega_q
    auto q_dot = (q * omega_q);

    q.w () += 0.5 * dt * q_dot.w (); // `0.5` for double-cover
    q.vec () += 0.5 * dt * q_dot.vec (); // `dt` bc it's unit speed integrated during the timestep
    q.normalize ();

    /*
    // or use exponential map
    auto axis = omega.normalized(); // axis = omega / omega.norm();
    auto angle = omega.norm() * dt; // unit speed multiplied with a discrete timestep size

    // so(3) -> SO(3), exp([omega])
    // so(3) -> S^3, quat_from_axis_angle(angle, axis)
    auto q_change = Quaternionr(); // q_change = [cos(angle/2); sin(angle/2) * axis];
    q_change.w() = cos(angle/2);
    q_change.vec() = sin(angle/2) * axis;
    // or more compact
    // auto q_change = Eigen::Quaterniond(Eigen::AngleAxisd(angle, axis));

    q = q * q_change; // q_change is derived from local quantity, so right multiply
    // hamiltonian product between two unit quats give you another unit quat, so skip normalization
    q.normalize(); // (..or do we?)
    */
}

inline void rb_update_velocity (Vector3r& v,
Vector3r& v_old,
Vector3r& omega,
Vector3r& omega_old,
const Vector3r& x,
const Vector3r& x_old,
const Quaternionr& q,
const Quaternionr& q_old,
const Real dt) {
    v_old     = v;
    omega_old = omega;

    v = (x - x_old) / dt;

    // again, omega is a body quantity, so change goes to the right side
    // q_new = q_old * q_change => inv(q_old) * q_new = q_change
    auto q_change = q_old.conjugate () * q;

    // inverse op
    omega = 2.0 * q_change.vec () / dt;

    // send to antipodal, since we want the shortest path
    if (q_change.w () < 0.0) {
        omega = -omega;
    }
}

inline void rb_reset (Vector3r& x,
Quaternionr& q,
Vector3r& v,
Vector3r& omega,
const Vector3r& x0,
const Quaternionr& q0,
const Vector3r& v0,
const Vector3r& omega0) {
    // set body state to its initial condition
    x     = x0;
    q     = q0;
    v     = v0;
    omega = omega0;
}

// poco
inline void rb_clear_force (Vector3r& f, Vector3r& tau) {
    f.setZero ();
    tau.setZero ();
}

// entity function
inline Vector3r rb_calculate_external_force (flecs::entity e) {
    const Vector3r center_of_mass (0, 0, 0);
    Vector3r total_force (0, 0, 0);
    auto forces = e.get<Forces> ()->a;
    for (int i = 0; i < forces.size (); ++i) {
        total_force += forces[i].force;
    }

    return total_force;
}

// entity function
inline Vector3r rb_calculate_external_torque (flecs::entity e) {
    const Vector3r center_of_mass (0, 0, 0);
    Vector3r total_torque (0, 0, 0);
    auto forces = e.get<Forces> ()->a;
    for (int i = 0; i < forces.size (); ++i) {
        auto application_point = forces[i].position - center_of_mass;
        total_torque += application_point.cross (forces[i].force);
    }

    return total_torque;
}

// entity function
inline void rb_clear_constraint_lambda (Constraint& c) {
    switch (c.type) {
    case POSITIONAL_CONSTRAINT: {
        c.positional_constraint.lambda = 0.0;
        return;
    } break;
        // case COLLISION_CONSTRAINT: {
        //     c.collision_constraint.lambda_n = 0.0;
        //     c.collision_constraint.lambda_t = 0.0;
        //     return;
        // } break;
        // case SPHERICAL_JOINT_CONSTRAINT: {
        //     c.spherical_joint_constraint.lambda_pos   = 0.0;
        //     c.spherical_joint_constraint.lambda_swing = 0.0;
        //     c.spherical_joint_constraint.lambda_twist = 0.0;
        // }
    }

    assert (0);
}

inline void rb_identify_island (flecs::iter& it, Real dt) {
    std::vector<flecs::entity> entities;
    // entities.reserve (it.count ());

    while (it.next ()) {
        for (auto i : it) {
            entities.push_back (it.entity (i));
        }
    }


    auto broad_collision_pairs = broad_get_collision_pairs (entities);

    // do the island sleep thing
    auto simulation_islands =
    broad_collect_simulation_islands (entities, broad_collision_pairs);
    for (size_t j = 0; j < simulation_islands.size (); ++j) {
        auto simulation_island = simulation_islands[j];

        bool all_inactive = true;
        for (size_t k = 0; k < simulation_island.size (); ++k) {
            auto e = simulation_island[k];

            auto linear_velocity_norm = e.get<LinearVelocity> ()->value.norm ();
            auto angular_velocity_norm = e.get<AngularVelocity> ()->value.norm ();
            if (linear_velocity_norm < LINEAR_SLEEPING_THRESHOLD &&
            angular_velocity_norm < ANGULAR_SLEEPING_THRESHOLD) {
                e.get_mut<DeactivationTime> ()->value += dt;
            } else {
                e.get_mut<DeactivationTime> ()->value = 0.0;
            }

            if (e.get_mut<DeactivationTime> ()->value < DEACTIVATION_TIME_TO_BE_INACTIVE) {
                all_inactive = false;
            }
        }

        // we only set entities to inactive if the whole island is inactive!
        for (size_t k = 0; k < simulation_island.size (); ++k) {
            auto e = simulation_island[k];
            // e.get_mut<IsActive> ()->value = !all_inactive;
            if (all_inactive) {
                e.add<IsActive> ();
            } else {
                e.remove<IsActive> ();
            }
        }
    }

    // redundant
    broad_simulation_islands_destroy (simulation_islands);
}

static void clipping_contact_to_collision_constraint (flecs::entity e1,
flecs::entity e2,
Collider_Contact& contact,
Constraint& constraint) {
    constraint.type                          = COLLISION_CONSTRAINT;
    constraint.e1                            = e1;
    constraint.e2                            = e2;
    constraint.collision_constraint.normal   = contact.normal;
    constraint.collision_constraint.lambda_n = 0.0;
    constraint.collision_constraint.lambda_t = 0.0;

    auto r1_wc = contact.collision_point1 - e1.get<Position> ()->value;
    auto r2_wc = contact.collision_point2 - e2.get<Position> ()->value;

    constraint.collision_constraint.r1_lc =
    e1.get<Orientation> ()->value.toRotationMatrix ().transpose () * r1_wc;
    constraint.collision_constraint.r2_lc =
    e2.get<Orientation> ()->value.toRotationMatrix ().transpose () * r2_wc;
}

inline void rb_collect_collision (flecs::iter& it) {
    std::vector<flecs::entity> entities;

    while (it.next ()) {
        for (auto i : it) {
            entities.push_back (it.entity (i));
        }
    }

    auto broad_collision_pairs = broad_get_collision_pairs (entities);

    for (size_t i = 0; i < broad_collision_pairs.size (); ++i) {
        auto e1 = broad_collision_pairs[i].e1;
        auto e2 = broad_collision_pairs[i].e2;

        auto colliders1 = e1.get_mut<Colliders> ()->value;
        auto colliders2 = e2.get_mut<Colliders> ()->value;

        if (!e1.has<IsPinned> () && !e2.has<IsPinned> ()) {
            assert ((e1.has<IsActive> () && e2.has<IsActive> ()) ||
            (!e1.has<IsActive> () && !e2.has<IsActive> ()));
        }

        if ((e1.has<IsPinned> () || !e1.has<IsActive> ()) &&
        (e2.has<IsPinned> () || !e2.has<IsActive> ())) {
            continue;
        }

        if (colliders1.empty () || colliders2.empty ()) {
            continue;
        }

        colliders_update (
        colliders1, e1.get<Position> ()->value, &e1.get<Orientation> ()->value);
        colliders_update (
        colliders2, e2.get<Position> ()->value, &e2.get<Orientation> ()->value);

        auto contacts = colliders_get_contacts (colliders1, colliders2);

        for (size_t l = 0; l < contacts.size (); ++l) {
            Collider_Contact contact = contacts[l];
            Constraint constraint;
            clipping_contact_to_collision_constraint (e1, e2, contact, constraint);
            auto new_constraint_entity = it.world ().entity ();
            new_constraint_entity.set<Constraint> ({ constraint });
        }
    }
}

// entity function
void rb_positional_constraint_init (Constraint& constraint,
flecs::entity e1,
flecs::entity e2,
Vector3r r1_lc,
Vector3r r2_lc,
Real compliance,
Real distance) {
    constraint.type                             = POSITIONAL_CONSTRAINT;
    constraint.e1                               = e1;
    constraint.e2                               = e2;
    constraint.positional_constraint.r1_lc      = r1_lc;
    constraint.positional_constraint.r2_lc      = r2_lc;
    constraint.positional_constraint.compliance = compliance;
    constraint.positional_constraint.distance   = distance;
}

static void positional_constraint_solve (Constraint& constraint, Real h) {
    assert (constraint.type == POSITIONAL_CONSTRAINT);

    auto e1 = constraint.e1;
    auto e2 = constraint.e2;

    auto x1 = e1.get_mut<Position> ()->value;
    auto x2 = e2.get_mut<Position> ()->value;

    auto delta_x = x2 - x1; // delta starts from `x1` and ends to `x2`
    auto violation = delta_x.norm () - constraint.positional_constraint.distance;

    Position_Constraint_Preprocessed_Data pcpd;
    calculate_positional_constraint_preprocessed_data (e1, e2,
    constraint.positional_constraint.r1_lc, constraint.positional_constraint.r2_lc, pcpd);

    auto delta_lambda = positional_constraint_get_delta_lambda (pcpd, h,
    constraint.positional_constraint.compliance,
    constraint.positional_constraint.lambda, delta_x, violation);

    positional_constraint_apply (pcpd, delta_lambda, delta_x);
    constraint.positional_constraint.lambda += delta_lambda;
}

static void collision_constraint_solve (Constraint& constraint, Real h) {
    assert (constraint.type == COLLISION_CONSTRAINT);

    auto e1 = constraint.e1;
    auto e2 = constraint.e2;

    Position_Constraint_Preprocessed_Data pcpd;
    calculate_positional_constraint_preprocessed_data (e1, e2,
    constraint.collision_constraint.r1_lc, constraint.collision_constraint.r2_lc, pcpd);

    // todo..
}

inline void rb_solve_constraint (Constraint& constraint, Real dt) {
    switch (constraint.type) {
    case POSITIONAL_CONSTRAINT: {
        positional_constraint_solve (constraint, dt);
        return;
    } break;
    case COLLISION_CONSTRAINT: {
        collision_constraint_solve (constraint, dt);
        return;
    } break;
    }

    assert (0);
}

} // namespace rigidbody
} // namespace pbd
} // namespace phys