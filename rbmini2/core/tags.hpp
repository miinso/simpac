// core/tags.hpp
#pragma once

namespace phys {
    namespace tags {
        // phases
        struct PreUpdate { };
        struct OnUpdate { };
        struct PostUpdate { };
        struct OnValidate { };

        // entity types
        struct RigidBody { };
        struct Constraint { };
        struct Collider { };
        struct Joint { };
    } // namespace tags
} // namespace phys