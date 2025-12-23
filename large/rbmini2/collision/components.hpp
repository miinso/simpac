// collision/components.hpp
#pragma once
#include "collision/types.hpp"
#include "sdf/types.hpp"

namespace phys {
    namespace components {
        struct BoundingVolume {
            collision::AABB aabb;
        };

        struct SDFCollider {
            sdf::SDFType type;
            sdf::SDFData data;
            Vector3r scale = Vector3r::Ones();
            bool invert_normal = false;

            SDFCollider() = default;

            SDFCollider(SDFCollider&& other) noexcept
                : type(other.type)
                , data(std::move(other.data))
                , scale(std::move(other.scale))
                , invert_normal(other.invert_normal) { }

            SDFCollider& operator=(SDFCollider&& other) noexcept {
                if (this != &other) {
                    type = other.type;
                    data = std::move(other.data);
                    scale = std::move(other.scale);
                    invert_normal = other.invert_normal;
                }
                return *this;
            }

            SDFCollider(const SDFCollider&) = delete;
            SDFCollider& operator=(const SDFCollider&) = delete;
        };

        struct CollisionProperties {
            float margin = 0.01f;
            float friction = 0.5f;
            float restitution = 0.5f;
            uint32_t collision_group = 1;
            uint32_t collision_mask = 0xFFFFFFFF;
        };
    } // namespace components
} // namespace phys