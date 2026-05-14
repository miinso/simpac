#pragma once

#include "../particle/particle.h"

struct Constraint {};

struct DistanceConstraint {
    flecs::entity v0;
    flecs::entity v1;
    Real rest_length;
    Real stiffness;
    Real lambda = 0;

    static void meta(flecs::world& ecs) {
        ecs.component<DistanceConstraint>()
            .member("v0", &DistanceConstraint::v0)
            .member("v1", &DistanceConstraint::v1)
            .member("rest_length", &DistanceConstraint::rest_length)
            .member("stiffness", &DistanceConstraint::stiffness)
            .member("lambda", &DistanceConstraint::lambda);
    }
};

struct Spring {
    flecs::entity v0;
    flecs::entity v1;
    Real rest_length = 0;
    Real stiffness = 0;
    Real damping = 0;

    static void meta(flecs::world& ecs) {
        ecs.component<Spring>()
            .member("v0", &Spring::v0)
            .member("v1", &Spring::v1)
            .member("rest_length", &Spring::rest_length)
                .range(0.0, 10.0)
            .member("stiffness", &Spring::stiffness)
                .range(0.0, 200000.0)
            .member("damping", &Spring::damping)
                .range(0.0, 10.0);
    }
};

struct Triangle {
    flecs::entity v0;
    flecs::entity v1;
    flecs::entity v2;
    Eigen::Matrix2r dm_inv = Eigen::Matrix2r::Zero();
    Real area = 0;
    Real thickness = 1;
    Real mu = 0;
    Real lambda = 0;

    static void meta(flecs::world& ecs) {
        ecs.component<Triangle>()
            .member("v0", &Triangle::v0)
            .member("v1", &Triangle::v1)
            .member("v2", &Triangle::v2)
            .member("thickness", &Triangle::thickness)
            .member("mu", &Triangle::mu)
            .member("lambda", &Triangle::lambda);
    }
};

struct Hinge {
    flecs::entity v0;  // opposite vertex (triangle 0)
    flecs::entity v1;  // opposite vertex (triangle 1)
    flecs::entity v2;  // shared edge vertex
    flecs::entity v3;  // shared edge vertex
    Real rest_angle = 0;
    Real stiffness = 0;

    static void meta(flecs::world& ecs) {
        ecs.component<Hinge>()
            .member("v0", &Hinge::v0)
            .member("v1", &Hinge::v1)
            .member("v2", &Hinge::v2)
            .member("v3", &Hinge::v3)
            .member("rest_angle", &Hinge::rest_angle)
            .member("stiffness", &Hinge::stiffness);
    }
};

namespace cloth {

struct element {
    element(flecs::world& ecs) {
        ecs.component<Constraint>();
        DistanceConstraint::meta(ecs);
        Spring::meta(ecs);
        Triangle::meta(ecs);
        Hinge::meta(ecs);

        ecs.observer<Spring>()
            .event(flecs::OnSet)
            .event(flecs::OnAdd)
            .event(flecs::OnRemove)
            .each([](flecs::iter&, size_t, Spring&) { sim::model_dirty = true; });

        ecs.observer<Hinge>()
            .event(flecs::OnSet)
            .event(flecs::OnAdd)
            .event(flecs::OnRemove)
            .each([](flecs::iter&, size_t, Hinge&) { sim::model_dirty = true; });
    }
};

} // namespace cloth
