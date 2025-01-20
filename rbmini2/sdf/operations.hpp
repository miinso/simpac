// sdf/operations.hpp
#pragma once
#include "sdf/analytic.hpp"
#include "sdf/components.hpp"
#include "sdf/discrete.hpp"
#include "sdf/types.hpp"

namespace phys {
    namespace sdf {
        inline double distance(const components::SDFCollider& collider,
                               const Vector3r& point,
                               const Real tolerance) {
            // apply inverse scaling to the query point
            const Vector3r scaled_point = point.cwiseProduct(collider.scale.cwiseInverse());
            double dist;

            switch (collider.type) {
            case SDFType::Sphere: {
                const auto& sphere = collider.sphere;
                dist = scaled_point.norm() - sphere.radius;
                break;
            }
            case SDFType::Box: {
                const auto& box = collider.box;
                const Vector3r d = scaled_point.cwiseAbs() - box.half_extents;
                const Vector3r max_d = d.cwiseMax(Vector3r::Zero());
                dist = std::min(d.maxCoeff(), 0.0f) + max_d.norm();
                break;
            }
            case SDFType::Grid: {
                if (!collider.grid || !collider.grid->grid)
                    return std::numeric_limits<double>::max();

                dist = collider.grid->grid->interpolate(0, scaled_point.cast<double>());
                if (dist == std::numeric_limits<double>::max())
                    return dist;

                dist *= collider.scale[0]; // Scale correction for grid
                break;
            }
            default:
                return std::numeric_limits<double>::max();
            }

            return collider.invert_sdf * dist - tolerance;
        }

        inline Vector3r approximate_normal(const components::SDFCollider& collider,
                                           const Vector3r& point,
                                           const Real tolerance,
                                           const Real epsilon = 1.0e-6) {
            if (collider.type == SDFType::Grid) {
                assert(false); // never happenes
            }

            Vector3r normal;
            for (int i = 0; i < 3; i++) {
                Vector3r offset = Vector3r::Zero();
                offset[i] = epsilon;

                double dist_plus = distance(collider, point + offset, tolerance);
                double dist_minus = distance(collider, point - offset, tolerance);

                normal[i] = static_cast<Real>((dist_plus - dist_minus) / (2.0 * epsilon));
            }

            const Real norm = normal.norm();
            if (norm < 1.0e-6)
                return Vector3r::Zero();

            return normal / norm;
        }

        inline bool collision_test(const components::SDFCollider& collider,
                                   const Vector3r& point,
                                   const Real tolerance,
                                   Vector3r& contact_point,
                                   Vector3r& normal,
                                   Real& dist,
                                   const Real max_dist = 0.0) {
            // apply inverse scaling to the query point
            const Vector3r scaled_point = point.cwiseProduct(collider.scale.cwiseInverse());

            if (collider.type == SDFType::Grid) {
                if (!collider.grid || !collider.grid->grid)
                    return false;

                Eigen::Vector3d normal_d;
                double d =
                    collider.grid->grid->interpolate(0, scaled_point.cast<double>(), &normal_d);

                if (d == std::numeric_limits<double>::max())
                    return false;

                // Apply scale to distance
                d *= collider.scale[0];

                dist = static_cast<Real>(collider.invert_sdf * d - tolerance);

                if (dist >= max_dist)
                    return false;

                // Apply invert_sdf to normal direction
                normal_d *= collider.invert_sdf;
                normal_d.normalize();
                normal = normal_d.cast<Real>();

                // Calculate contact point in scaled space then transform back
                contact_point = (scaled_point - dist * normal).cwiseProduct(collider.scale);
                return true;
            }

            // For other SDF types
            dist = static_cast<Real>(distance(collider, point, tolerance));
            if (dist >= max_dist)
                return false;

            normal = approximate_normal(collider, point, tolerance);
            contact_point = point - dist * normal;
            return true;
        }
    } // namespace sdf
} // namespace phys