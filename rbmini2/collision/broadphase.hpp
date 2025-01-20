// collision/broadphase.hpp
#pragma once

#include "collision/types.hpp"
#include <vector>

namespace phys {
    namespace collision {
        inline AABB compute_mesh_aabb(const std::vector<Vector3r>& vertices,
                                      const Vector3r& position,
                                      const Quaternionr& orientation,
                                      const Vector3r& scale,
                                      const Real margin = 0.0f) {

            AABB aabb;
            if (vertices.empty())
                return aabb;

            Matrix3r R = orientation.matrix();
            Vector3r transformed = R * (vertices[0].cwiseProduct(scale)) + position;
            aabb.min = aabb.max = transformed;

            for (size_t i = 1; i < vertices.size(); ++i) {
                transformed = R * (vertices[i].cwiseProduct(scale)) + position;
                aabb.min = aabb.min.cwiseMin(transformed);
                aabb.max = aabb.max.cwiseMax(transformed);
            }

            Vector3r margin_vec(margin, margin, margin);
            aabb.min -= margin_vec;
            aabb.max += margin_vec;
            return aabb;
        }
    } // namespace collision
} // namespace phys