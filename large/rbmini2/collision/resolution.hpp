// collision/resolution.hpp
#pragma once
#include "collision/types.hpp"

namespace phys {
    namespace collision {
        inline void compute_contact_jacobian(ContactConstraint& constraint,
                                             const Vector3r& pos1,
                                             const Vector3r& pos2,
                                             Eigen::Matrix<phys::Real, 3, 6>& J1,
                                             Eigen::Matrix<phys::Real, 3, 6>& J2) {

            const auto& contact = constraint.contact;
            Vector3r r1 = contact.position - pos1;
            Vector3r r2 = contact.position - pos2;

            // normal components
            J1.block<1, 3>(0, 0) = contact.normal.transpose();
            J1.block<1, 3>(0, 3) = r1.cross(contact.normal).transpose();

            // tangent component
            J1.block<1, 3>(1, 0) = contact.tangent1.transpose();
            J1.block<1, 3>(1, 3) = r1.cross(contact.tangent1).transpose();

            // bitangent component
            J1.block<1, 3>(2, 0) = contact.tangent2.transpose();
            J1.block<1, 3>(2, 3) = r1.cross(contact.tangent2).transpose();

            // reversal for J2
            // J2 = -J1;
        }

        inline void solve_contact_constraint_pgs(
            ContactConstraint& constraint,
            const Real dt,
            const Real inv_mass1,
            const Real inv_mass2,
            const Matrix3r& inv_inertia1,
            const Matrix3r& inv_inertia2,
            Vector3r& lambda) {

            // TODO: do PGS
        }
    } // namespace collision
} // namespace phys