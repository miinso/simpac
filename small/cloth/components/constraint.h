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

struct Spring {
    flecs::entity e1;
    flecs::entity e2;
    Real rest_length = 0;
    Real k_s = 0;
    Real k_d = 0;

    static void meta(flecs::world& ecs) {
        ecs.component<Spring>()
            .member("e1", &Spring::e1)
            .member("e2", &Spring::e2)
            .member("rest_length", &Spring::rest_length)
                .range(0.0, 10.0)
            .member("k_s", &Spring::k_s)
                .range(0.0, 200000.0)
            .member("k_d", &Spring::k_d)
                .range(0.0, 10.0);
    }
};

struct Triangle {
    flecs::entity e1;
    flecs::entity e2;
    flecs::entity e3;
    Eigen::Matrix2r dm_inv = Eigen::Matrix2r::Zero(); // computed by Bridge if zero
    Real area = 0;                                     // computed by Bridge if zero
    Real thickness = 1;
    Real mu = 0;
    Real lambda = 0;

    static void meta(flecs::world& ecs) {
        ecs.component<Triangle>()
            .member("e1", &Triangle::e1)
            .member("e2", &Triangle::e2)
            .member("e3", &Triangle::e3)
            .member("thickness", &Triangle::thickness)
            .member("mu", &Triangle::mu)
            .member("lambda", &Triangle::lambda);
    }
};

namespace components {

inline void register_constraint_components(flecs::world& ecs) {
    ecs.component<Constraint>();
    Spring::meta(ecs);
    DistanceConstraint::meta(ecs);
    Triangle::meta(ecs);
}

} // namespace components
