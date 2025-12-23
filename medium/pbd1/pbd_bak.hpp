// rigid_body.hpp
#pragma once

#include <Eigen/Dense>
#include <flecs.h>

#include "components.hpp"

static const Eigen::Vector3f gravity(0.0f, -9.87f, 0.0f);
// static const Eigen::Vector3f gravity(0.0f, 0.0f, 0.0f);

// dynamic kinetic static

struct Scene {
    float timestep;
    int num_substeps;
    int solve_iter;
};

struct Dynamic { };
struct Kinematic { };
struct Static { };

struct Constraint { };

struct IterCount { };
struct Substeps {
    int value;
};
struct Lambda {
    float value;
};

struct Stiffness {
    float value;
};

struct Distance {
    float value;
};

struct Angle {
    float value;
};

struct TwoTuple {
    flecs::entity e1;
    flecs::entity e2;
};

struct FourTuple {
    flecs::entity e1;
    flecs::entity e2;
    flecs::entity e3;
    flecs::entity e4;
};

struct RestDistance {
    float value;
};

struct RestVolume {
    float value;
};

struct DistanceConstraint {
    flecs::entity e1;
    flecs::entity e2;
    float rest_length;
    float stiffness;
    float lambda;
    float C;

    void solve() {
        // e1.get<Mass>();
    }
};

struct VolumeConstraint { };

void pbd_particle_clear_acceleration(Acceleration& a) {
    a.value = gravity;
}

void pbd_particle_integrate(flecs::iter& it,
                            size_t i,
                            Position& x,
                            OldPosition& x_old,
                            Mass& mass,
                            Velocity& v,
                            Acceleration& a) {
    const float dt = it.delta_time();

    x_old.value = x.value;

    v.value += dt * a.value;
    x.value += dt * v.value;
}

void pbd_constraint_solve1(flecs::iter& it, size_t i, Constraint& constraint) {
    auto max_iter = it.world().get<IterCount>();
    //  if (constraint.)
    auto e = it.entity(i);
    if (e.has<DistanceConstraint>()) {
        // pbd_solve_distance_constraint();
        // constraint.solve();
        auto c = e.get_mut<DistanceConstraint>();
        c->solve();

    } else
        (e.has<VolumeConstraint>()) {
            pbd_solve_volume_constraint();
        }
};

void pbd_solve_distance_constraint(
    flecs::iter& it, size_t, DistanceConstraint& constraint, const Scene& scene) {
    auto dt = scene.timestep / scene.num_substeps;

    auto e1 = constraint.e1;
    auto e2 = constraint.e2;

    auto x1 = e1.get_mut<Position>();
    auto x2 = e2.get_mut<Position>();

    auto w1 = e1.get<InverseMass>();
    auto w2 = e2.get<InverseMass>();

    auto w = w1->value + w2->value;

    auto delta = x2->value - x1->value;
    auto distance = delta.norm();
    auto grad = delta.normalize();

    auto C = distance - constraint.rest_length;

    float alpha = 1.0f / (constraint.stiffness * dt * dt);
    float delta_lambda = -(C + alpha * constraint.lambda) / (w1->value + w2->value + alpha);
    constraint.lambda += delta_lambda;

    x1->value += w1->value * delta_lambda * grad;
    x2->value -= w2->value * delta_lambda * grad;
}

flecs::entity add_distance_constraint(flecs::world& ecs, flecs::entity& e1, flecs::entity& e2) {
    auto e = ecs.entity();

    e.add<Constraint>();

    e.set<TwoTuple>({e1, e2})
        .add<DistanceConstraint>()
        .add<RestDistance>()
        .add<Lambda>()
        .add<Stiffness>();

    e.set<RestVolume>({1});
    e.set<Lambda>({0});
    e.set<Stiffness>({1});

    return e;
}

flecs::entity add_distance_constraint2(flecs::world& ecs, flecs::entity& e1, flecs::entity& e2) {
    auto e = ecs.entity();
    e.add<Constraint>();

    e.add<DistanceConstraint>();
    auto c = e.get_mut<DistanceConstraint>();

    c->lambda = 1;
    c->rest_length = 1;
    c->stiffness = 1;

    return e;
}

flecs::entity add_volume_constraint(
    flecs::world& ecs, flecs::entity& e1, flecs::entity& e2, flecs::entity& e3, flecs::entity& e4) {
    auto e = ecs.entity();

    e.add<Constraint>();

    e.set<FourTuple>({e1, e2, e3, e4})
        .add<VolumeConstraint>()
        .add<RestVolume>()
        .add<Lambda>()
        .add<Stiffness>();

    e.set<RestVolume>({1});
    e.set<Lambda>({0});
    e.set<Stiffness>({1});

    return e;
}

flecs::entity add_particle(flecs::world& ecs) {
    auto e = ecs.entity();

    e.add<Particle>();

    e.add<Position>()
        .add<OldPosition>()
        .add<Velocity>()
        .add<Acceleration>()
        .add<Mass>()
        .add<InverseMass>();

    e.set<Position>({Eigen::Vector3f::Zero()});
    e.set<OldPosition>({Eigen::Vector3f::Zero()});
    e.set<Velocity>({Eigen::Vector3f::Zero()});
    e.set<Acceleration>({Eigen::Vector3f::Zero()});

    e.set<Mass>({1.0f});
    e.set<InverseMass>({1.0f});

    return e;
}

void thing(flecs::world& ecs) {
    int max_iter = 10;

    ecs.system<Constraint>().run([&](flecs::iter& it) {
        for (size_t i = 0; i < max_iter; ++i) {
            while (it.next()) {
            }
        }
    });
}

void pbd_particle_damp_velocity(flecs::iter& it, size_t i) {
    // mass weighted position
    // mass weighted velocity
    // angular momentum
    // linear momentum
    // angular velocity

    // for all vertices
    //      filter velocity
    // => acts more like a part of rigid body
    // => less individual spikes
}