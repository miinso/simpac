#pragma once

#include <Eigen/Dense>
#include <flecs.h>
#include <iostream>
#include <map>

#include "components.hpp"

struct BodyPairKey {
    flecs::entity e1;
    flecs::entity e2;

    bool operator<(const BodyPairKey& other) const {
        if (e1 < other.e1)
            return true;
        if (e1 > other.e1)
            return false;
        return e2 < other.e2;
    }
};

// compute right hand side of the system for a contact
void compute_rhs(Contact& contact, Vector3r& b, float inv_dt) {
    const float gamma = 0.3f; // constraint softness

    // b = -phi/h - J*v
    // normal component
    b[0] = -inv_dt * gamma * contact.dist;
    b[1] = 0.0f; // tangent directions
    b[2] = 0.0f;

    // add velocity terms
    if (!contact.e1.has<IsPinned>()) {
        auto vel1 = contact.e1.get<LinearVelocity>();
        auto omega1 = contact.e1.get<AngularVelocity>();

        // -J*v for body 1
        b -= contact.J0.block<3, 3>(0, 0) * vel1->value;
        b -= contact.J0.block<3, 3>(0, 3) * omega1->value;
    }

    if (!contact.e2.has<IsPinned>()) {
        auto vel2 = contact.e2.get<LinearVelocity>();
        auto omega2 = contact.e2.get<AngularVelocity>();

        // -J*v for body 2
        b -= contact.J1.block<3, 3>(0, 0) * vel2->value;
        b -= contact.J1.block<3, 3>(0, 3) * omega2->value;
    }
}

// compute the diagonal term of the system matrix for a contact
void compute_diagonal(Contact& contact, Matrix3r& diagonal) {
    diagonal.setZero();

    // A = J*M^-1*J^T
    if (!contact.e1.has<IsPinned>()) {
        diagonal += contact.J0_Minv * contact.J0.transpose();
    }

    if (!contact.e2.has<IsPinned>()) {
        diagonal += contact.J1_Minv * contact.J1.transpose();
    }

    // add small regularization term for stability
    diagonal += Matrix3r::Identity() * 1e-6f;
}

// solve 3x3 box constrained system for single contact
void solve_contact_block(Matrix3r& A, Vector3r& b, Vector3r& x, float friction) {
    // project normal component to [0, inf]
    x[0] = (b[0] - A(0, 1) * x[1] - A(0, 2) * x[2]) / A(0, 0);
    x[0] = std::max(0.0f, x[0]);

    // project friction forces to friction cone: |ft| <= mu*fn
    const float limit = friction * x[0];

    x[1] = (b[1] - A(1, 0) * x[0] - A(1, 2) * x[2]) / A(1, 1);
    x[1] = (x[1] < -limit) ? -limit : ((x[1] > limit) ? limit : x[1]);

    x[2] = (b[2] - A(2, 0) * x[0] - A(2, 1) * x[1]) / A(2, 2);
    x[2] = (x[2] < -limit) ? -limit : ((x[2] > limit) ? limit : x[2]);
}

// main solver function
void solve_contacts_pgs(flecs::iter& it) {
    const int max_iter = 5;
    const float dt = it.delta_time();
    const float inv_dt = 1.0f / it.delta_time();

    std::map<BodyPairKey, int> contact_counts;
    it.world().each<Contact>([&contact_counts](flecs::entity e, Contact& contact) {
        BodyPairKey key{contact.e1, contact.e2};
        contact_counts[key]++;
    });

    // PGS iteration
    // TODO: implement main PGS loop here
    // 1. compute RHS vector b for each contact
    // 2. iterate to solve for contact forces (lambda)
    // 3. apply computed forces back to rigid bodies

    // early exit if no contacts?

    // temporary storage for solving
    std::vector<Vector3r> b_vector;
    std::vector<Matrix3r> A_diag;

    // collect all contacts and build system
    it.world().each<Contact>([&](flecs::entity e, Contact& contact) {
        Vector3r b;
        Matrix3r diag;
        compute_rhs(contact, b, inv_dt);
        compute_diagonal(contact, diag);

        b_vector.push_back(b);
        A_diag.push_back(diag);
        contact.lambda.setZero();
    });

    // PGS iteration
    for (int iter = 0; iter < max_iter; iter++) {
        int contact_idx = 0;
        it.world().each<Contact>([&](flecs::entity e, Contact& contact) {
            Vector3r& b = b_vector[contact_idx];
            Matrix3r& A = A_diag[contact_idx];

            // solve 3x3 block for this contact
            solve_contact_block(A, b, contact.lambda, contact.friction);
            contact_idx++;
        });
    }

    // apply constraint forces
    it.world().each<Contact>([&](flecs::entity e, Contact& contact) {
        // get contact count for this body pair
        BodyPairKey key{contact.e1, contact.e2};
        float contact_scale = 1.0f / static_cast<float>(contact_counts[key]);

        if (!contact.e1.has<IsPinned>()) {
            auto force1 = contact.e1.get_mut<LinearForce>();
            auto torque1 = contact.e1.get_mut<AngularForce>();

            Vector6r f1 = contact.J0.transpose() * contact.lambda / dt * contact_scale;
            force1->value += f1.head<3>();
            torque1->value += f1.tail<3>();
        }

        if (!contact.e2.has<IsPinned>()) {
            auto force2 = contact.e2.get_mut<LinearForce>();
            auto torque2 = contact.e2.get_mut<AngularForce>();

            Vector6r f2 = contact.J1.transpose() * contact.lambda / dt * contact_scale;
            force2->value += f2.head<3>();
            torque2->value += f2.tail<3>();
        }
    });
}
