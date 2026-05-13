#pragma once

#include "particle.h"

struct Constraint {};

struct DistanceConstraint {
    flecs::entity e1;
    flecs::entity e2;
    Real rest_distance;
    Real stiffness;
    Real lambda = 0;

    static void meta(flecs::world& ecs) {
        ecs.component<DistanceConstraint>()
            .member("e1", &DistanceConstraint::e1)
            .member("e2", &DistanceConstraint::e2)
            .member("rest_distance", &DistanceConstraint::rest_distance)
            .member("stiffness", &DistanceConstraint::stiffness)
            .member("lambda", &DistanceConstraint::lambda);
    }
};

namespace components {

inline void register_constraint_components(flecs::world& ecs) {
    ecs.component<Constraint>("::Constraint");
    ecs.component<DistanceConstraint>("::DistanceConstraint");
    DistanceConstraint::meta(ecs);
}

} // namespace components
