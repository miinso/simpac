// rigid_body.hpp
#pragma once

#include <Eigen/Dense>
#include <flecs.h>

#include "components.hpp"

static constexpr float _EPSILON = 1e-6f;

struct Constraint { };

struct DistanceConstraint {
    flecs::entity e1;
    flecs::entity e2;
    float rest_distance;
    float stiffness;
    float lambda = 0;

    void solve(float dt) {
        auto x1 = e1.get_mut<Position>();
        auto x2 = e2.get_mut<Position>();

        auto w1 = e1.get<InverseMass>();
        auto w2 = e2.get<InverseMass>();

        auto w = w1->value + w2->value;
        if (w < _EPSILON)
            return;

        auto delta = x1->value - x2->value;
        auto distance = delta.norm();

        if (distance < _EPSILON)
            return;

        auto grad = delta / distance;

        auto C = distance - rest_distance;
        float alpha = 1.0f / (stiffness * dt * dt);
        float delta_lambda = -(C + alpha * lambda) / (w + alpha);
        // lambda = -s?
        lambda += delta_lambda;

        x1->value += w1->value * delta_lambda * grad;
        x2->value += w2->value * delta_lambda * -grad;
    }
};

void pbd_particle_clear_acceleration(flecs::iter& it, size_t, Acceleration& a) {
    auto params = it.world().get<Scene>();
    a.value = params->gravity;
}

void pbd_particle_clear_constraint_lambda(flecs::iter& it, size_t, DistanceConstraint& c) {
    c.lambda = 0;

    // if (e.has<DistanceConstraint>()) {
    //     e.get_mut<DistanceConstraint>()->lambda = 0;
    // }
    // while (it.next()) {
    //     for (auto index : it) {
    //         auto e = it.entity(index);
    //         if (e.has<DistanceConstraint>()) {
    //             auto c = e.get_mut<DistanceConstraint>();
    //             c->lambda = 0;
    //         }
    //     }
    // }
}

void pbd_particle_integrate(flecs::iter& it,
                            size_t i,
                            Position& x,
                            OldPosition& x_old,
                            Mass& mass,
                            Velocity& v,
                            Acceleration& a) {
    auto params = it.world().get<Scene>();
    float dt = params->timestep / params->num_substeps;

    // if (it.entity(i).has<IsPinned>()) return;

    x_old.value = x.value;

    v.value += dt * a.value;
    x.value += dt * v.value;
}

// run, not each
// ecs.system<Constraint>().run([](flecs::iter& it) {
void pbd_particle_solve_constraint(flecs::iter& it, size_t, DistanceConstraint& c) {
    auto params = it.world().get<Scene>();
    float dt = params->timestep / params->num_substeps;

    // it.next() is some special concept employed in flecs. it's not that cpp iter
    // maybe hopping over some virtual tables that is match to our query?
    // while (it.next()) {
    //     for (int i = 0; i < params->solve_iter; ++i) {
    //         for (auto index : it) {
    //             auto e = it.entity(index);
    //             if (e.has<DistanceConstraint>()) {
    //                 // assert(false);
    //                 auto c = e.get_mut<DistanceConstraint>();
    //                 c->solve(dt);
    //             }
    //         }
    //     }
    // }
    c.solve(dt);
};

void pbd_particle_ground_collision(flecs::iter& it, size_t, Position& x, const OldPosition& x_old) {
    auto params = it.world().get<Scene>();
    float dt = params->timestep / params->num_substeps;

    const float friction = 0.8f;

    if (x.value.y() < 0) {
        x.value.y() = 0;

        auto displacement = x.value - x_old.value;

        auto friction_factor = std::min(1.0f, dt * friction);
        x.value.x() = x_old.value.x() + displacement.x() * (1 - friction_factor);
        x.value.z() = x_old.value.z() + displacement.z() * (1 - friction_factor);
    }
}

void pbd_particle_update_velocity(
    flecs::iter& it, size_t, const Position& x, const OldPosition& x_old, Velocity& v) {
    auto params = it.world().get<Scene>();
    float dt = params->timestep / params->num_substeps;

    v.value = (x.value - x_old.value) / dt;
}

// merging and splitting idea from muller2006position. See $3.5. Damping
// void pbd_particle_damp_velocity(const Mass& m, const Position& x, const Velocity& v) {}

// `step` requires particle query, and constraint query
// void pbd_step(flecs::iter& it) {
//     auto params = it.world().get<Scene>();
//     float dt = params->timestep / params->num_substeps;

//     // for all particles
//     while (it.next()) {
//         for (int i = 0; i < params->num_substeps; ++i) {

//         }
//     }
// }

flecs::entity add_distance_constraint(
    flecs::world& ecs, flecs::entity& e1, flecs::entity& e2, float stiffness = 1.0f) {

    auto rest_distance = (e1.get<Position>()->value - e2.get<Position>()->value).norm();

    auto e = ecs.entity();
    e.add<Constraint>();

    e.add<DistanceConstraint>();
    auto c = e.get_mut<DistanceConstraint>();
    c->e1 = e1;
    c->e2 = e2;
    c->rest_distance = rest_distance;
    c->stiffness = stiffness;

    return e;
}

flecs::entity add_particle(flecs::world& ecs,
                           const Eigen::Vector3f& pos = Eigen::Vector3f::Zero(),
                           float mass = 1.0f) {
    auto e = ecs.entity();

    e.add<Particle>();

    e.add<Position>()
        .add<OldPosition>()
        .add<Velocity>()
        .add<Acceleration>()
        .add<Mass>()
        .add<InverseMass>();

    e.set<Position>({pos});
    e.set<OldPosition>({pos});
    e.set<Velocity>({Eigen::Vector3f::Zero()});
    e.set<Acceleration>({Eigen::Vector3f::Zero()});

    e.set<Mass>({1.0f});
    e.set<InverseMass>({1.0f});

    return e;
}
