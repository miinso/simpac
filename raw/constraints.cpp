#include "constraints.h"

using namespace phys::pbd;

void calculate_positional_constraint_preprocessed_data (flecs::entity e1,
flecs::entity e2,
Vector3r r1_lc,
Vector3r r2_lc,
Position_Constraint_Preprocessed_Data& pcpd) {
    pcpd.e1 = e1;
    pcpd.e2 = e2;

    const auto R1 = e1.get<Orientation> ()->value.toRotationMatrix ();
    const auto R2 = e2.get<Orientation> ()->value.toRotationMatrix ();

    const auto I_body_inv1 = e1.get<LocalInverseInertia> ()->value;
    const auto I_body_inv2 = e2.get<LocalInverseInertia> ()->value;

    pcpd.r1_wc = R1 * r1_lc;
    pcpd.r2_wc = R2 * r2_lc;

    pcpd.e1_world_inverse_inertia_tensor = R1 * I_body_inv1 * R1.transpose ();
    pcpd.e2_world_inverse_inertia_tensor = R2 * I_body_inv2 * R2.transpose ();
}

Real positional_constraint_get_delta_lambda (Position_Constraint_Preprocessed_Data& pcpd,
Real h,
Real compliance,
Real lambda,
Vector3r delta_x,
Real violation) {
    // evaluate the constraint function for distance constraint
    // violation = |l - l0|
    // Real violation = delta_x.norm ();

    // const Real EPSILON = 1e-12;
    if (violation <= 1e-12) {
        return 0.0;
    }

    auto e1           = pcpd.e1;
    auto e2           = pcpd.e2;
    auto r1_wc        = pcpd.r1_wc; // application point in world coord
    auto r2_wc        = pcpd.r2_wc;
    auto I_world_inv1 = pcpd.e1_world_inverse_inertia_tensor;
    auto I_world_inv2 = pcpd.e2_world_inverse_inertia_tensor;

    auto n = delta_x.normalized (); // direction in world coord

    // generalized inversed mass for each body
    auto w1 = e1.get<Mass> ()->value +
    r1_wc.cross (n).transpose () * I_world_inv1 * r1_wc.cross (n);
    auto w2 = e2.get<Mass> ()->value +
    r2_wc.cross (n).transpose () * I_world_inv2 * r2_wc.cross (n);

    assert (w1 + w2 != 0.0);

    Real alpha_tilde = compliance / (h * h);
    Real delta_lambda = -(violation + alpha_tilde * lambda) / (w1 + w2 + alpha_tilde);

    return delta_lambda;
}

void positional_constraint_apply (Position_Constraint_Preprocessed_Data& pcpd,
Real delta_lambda,
Vector3r delta_x) {
    auto e1           = pcpd.e1;
    auto e2           = pcpd.e2;
    auto r1_wc        = pcpd.r1_wc; // application point in world coord
    auto r2_wc        = pcpd.r2_wc;
    auto I_world_inv1 = pcpd.e1_world_inverse_inertia_tensor;
    auto I_world_inv2 = pcpd.e2_world_inverse_inertia_tensor;
    auto& x1          = e1.get_mut<Position> ()->value;
    auto& x2          = e2.get_mut<Position> ()->value;
    auto& q1          = e1.get_mut<Orientation> ()->value;
    auto& q2          = e2.get_mut<Orientation> ()->value;

    auto n = delta_x.normalized (); // x1 to x2. direction in world coord

    auto grad_x1 = -n; // violation increases when x1 move in opposite to `n`
    auto grad_x2 = n;

    if (!e1.has<IsPinned> ()) {
        auto correction = e1.get<InverseMass> ()->value * delta_lambda * grad_x1;
        x1 += correction;
    }

    if (!e2.has<IsPinned> ()) {
        x2 += e2.get<InverseMass> ()->value * delta_lambda * grad_x2;
    }

    auto grad_phi1 = r1_wc.cross (-n);
    auto grad_phi2 = r2_wc.cross (n);


    if (!e1.has<IsPinned> ()) {
        // this time, the correction `q_change` is calculated from the world
        // quantities `I_world` and `n`, so left multiply.

        // correction = omega * dt

        // we know the amount of angular correction via L (delta_lambda *
        // r.cross(n)) inversely calculate omega from L. L / dt = omega tau * dt
        // = impulse, tau = I tau = I * alpha

        auto omega_dt_q   = Quaternionr ();
        omega_dt_q.w ()   = 0;
        omega_dt_q.vec () = I_world_inv1 * delta_lambda * grad_phi1;

        auto q_dot_dt = (omega_dt_q * q1);
        q1.w () += 0.5 * q_dot_dt.w ();
        q1.vec () += 0.5 * q_dot_dt.vec ();
        q1.normalize ();
    }

    if (!e2.has<IsPinned> ()) {
        auto omega_dt_q   = Quaternionr ();
        omega_dt_q.w ()   = 0;
        omega_dt_q.vec () = I_world_inv2 * delta_lambda * grad_phi2;

        auto q_dot_dt = (omega_dt_q * q2);
        q2.w () += 0.5 * q_dot_dt.w ();
        q2.vec () += 0.5 * q_dot_dt.vec ();
        q2.normalize ();
    }
}

void calculate_angular_constraint_preprocessed_data (flecs::entity e1,
flecs::entity e2,
Angular_Constraint_Preprocessed_Data& acpd) {
    acpd.e1 = e1;
    acpd.e2 = e2;

    const auto R1 = e1.get<Orientation> ()->value.toRotationMatrix ();
    const auto R2 = e2.get<Orientation> ()->value.toRotationMatrix ();

    const auto I_body_inv1 = e1.get<LocalInverseInertia> ()->value;
    const auto I_body_inv2 = e2.get<LocalInverseInertia> ()->value;

    acpd.e1_world_inverse_inertia_tensor = R1 * I_body_inv1 * R1.transpose ();
    acpd.e2_world_inverse_inertia_tensor = R2 * I_body_inv2 * R2.transpose ();
}

Real angular_constraint_get_delta_lambda (Angular_Constraint_Preprocessed_Data& acpd,
Real h,
Real compliance,
Real lambda,
Vector3r delta_q) {
}