#pragma once

#include <Eigen/Dense>
#include <flecs.h>
#include <vector>

namespace physics {
    enum CollisionFilter { none = 0x00, player = 0x01, enemy = 0x02, more = 0x04 };

    struct Velocity {
        Eigen::Vector3f value;
    };

    struct DesiredVelocity {
        Eigen::Vector3f value;
    };

    struct AccelerationSpeed {
        float value;
    };

    struct Collider {
        float radius;
        bool correct_position;
        CollisionFilter collision_type; // type
        CollisionFilter collision_filter; // filter
    };

    // method 1. using relationships (poor)
    struct CollidedWith {};

    // method 2. dispatching a new entity for each collision
    // (marginally better than method 1)
    struct CollisionRecord {
        flecs::entity a;
        flecs::entity b;
    };

    // method 3. maintaining a global list (kind of works)
    struct CollisionRecordList {
        std::vector<CollisionRecord> records;
        std::vector<CollisionRecord>
                significant_collisions; // what does it mean, significant collision? is this to
                                        // differenciate broad/narrow results?? i
                                        // guess it is (but not sure of it)
                                        // no! it means a subset of detected collisions,
                                        // specifically collision between entities with different
                                        // collision tag player-enemy: significant. enemy-enemy:
                                        // non-significant.
    };
} // namespace physics
