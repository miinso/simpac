// collision/narrowphase.hpp
#pragma once
#include "collision/types.hpp"
#include "sdf/analytic.hpp"
#include "sdf/discrete.hpp"
// #include "sdf/components.hpp"

namespace phys {
    namespace collision {
        inline bool detect_analytical_collision(
            const Vector3r& point,
            const sdf::SDFData& params,
            int sdf_type,
            const Vector3r& scale,
            bool invert_normal,
            float tolerance,
            ContactPoint& contact) {

            Vector3r scaled_point = point.cwiseQuotient(scale);

            float dist;
            switch (sdf_type) {
            case 0: // Box
                dist = sdf::box_sdf(scaled_point, params.box);
                break;
            case 1: // Sphere
                dist = sdf::sphere_sdf(scaled_point, params.sphere);
                break;
            default:
                return false;
            }

            dist *= scale.x(); // isotropic scale
            if (invert_normal)
                dist = -dist;

            if (dist < tolerance) {
                contact.penetration = -dist;

                contact.normal = sdf::compute_gradient(scaled_point, params, sdf_type);
                if (invert_normal)
                    contact.normal = -contact.normal;

                contact.position = point;
                return true;
            }

            return false;
        }

        inline bool detect_grid_collision(const Vector3r& point,
                                          const sdf::GridSDF& grid,
                                          const Vector3r& scale,
                                          bool invert_normal,
                                          float tolerance,
                                          ContactPoint& contact) {

            Vector3r scaled_point = point.cwiseQuotient(scale);

            float dist = sdf::sample_grid_sdf(grid, scaled_point);
            if (dist == std::numeric_limits<float>::max())
                return false;

            dist *= scale.x();
            if (invert_normal)
                dist = -dist;

            if (dist < tolerance) {
                contact.penetration = -dist;
                contact.normal = sdf::sample_grid_sdf_gradient(grid, scaled_point);
                if (invert_normal)
                    contact.normal = -contact.normal;
                contact.position = point;
                return true;
            }

            return false;
        }
    } // namespace collision
} // namespace phys