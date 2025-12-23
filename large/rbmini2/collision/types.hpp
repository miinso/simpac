// collision/types.hpp
#pragma once
#include "core/types.hpp"
#include <flecs.h>

namespace phys {
    namespace collision {
        struct AABB {
            Vector3r min;
            Vector3r max;

            bool overlaps(const AABB& other) const {
                return (min.x() <= other.max.x() && max.x() >= other.min.x()) &&
                       (min.y() <= other.max.y() && max.y() >= other.min.y()) &&
                       (min.z() <= other.max.z() && max.z() >= other.min.z());
            }
        };

        struct ContactPoint {
            Vector3r position; // world
            Vector3r normal; // world
            float penetration;
            Vector3r tangent1;
            Vector3r tangent2;
        };

        struct ContactConstraint {
            flecs::entity entity1;
            flecs::entity entity2;
            ContactPoint contact;
            Real friction;
            Real restitution;
            Vector3r lambda; // (normal, tangent, bitangent)
        };

        struct CollisionPair {
            flecs::entity e1;
            flecs::entity e2;
        };
    } // namespace collision
} // namespace phys