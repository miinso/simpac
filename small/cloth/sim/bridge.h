#pragma once

#include "model.h"
#include "../element/element.h"
#include "../math/bending.h"
#include "../math/adjacency.h"
#include <unordered_map>

namespace physics {

// maps ECS entities <-> solver indices, handles gather/scatter
struct Bridge {
    std::vector<flecs::entity> particle_entities; // [particle_count]
    std::unordered_map<uint64_t, int> entity_to_index; // internal, for resolving Spring/Triangle refs

    Real default_bending_stiffness = Real(1.0);

    Model build(flecs::world& world) {
        particle_entities.clear();
        entity_to_index.clear();

        ModelBuilder mb;

        auto pq = world.query_builder<const Position, const Velocity, const Mass>()
            .with<Particle>()
            .build();
        pq.each([&](flecs::entity e, const Position& pos, const Velocity& vel, const Mass& mass) {
                Real m = mass;
                uint32_t flags = PARTICLE_FLAG_ACTIVE;
                // pin sets a flag, not zero-mass. inv_mass is derived in finalize.
                if (e.has<IsPinned>()) flags |= PARTICLE_FLAG_PINNED;

                int idx = mb.add_particle(pos.map(), vel.map(), m, flags);
                entity_to_index[e.id()] = idx;
                particle_entities.push_back(e);
            });

        world.query_builder<const Spring>()
            .build()
            .each([&](flecs::entity, const Spring& spring) {
                auto it_a = entity_to_index.find(spring.v0.id());
                auto it_b = entity_to_index.find(spring.v1.id());
                if (it_a == entity_to_index.end() || it_b == entity_to_index.end()) return;

                mb.add_spring(it_a->second, it_b->second,
                              spring.stiffness, spring.damping, spring.rest_length);
            });

        world.query_builder<Triangle>()
            .build()
            .each([&](flecs::entity, Triangle& tri) {
                auto it0 = entity_to_index.find(tri.v0.id());
                auto it1 = entity_to_index.find(tri.v1.id());
                auto it2 = entity_to_index.find(tri.v2.id());
                if (it0 == entity_to_index.end() ||
                    it1 == entity_to_index.end() ||
                    it2 == entity_to_index.end()) return;

                // first build: capture rest pose from current positions, cache on the component.
                // subsequent builds reuse the cached values so the rest pose isn't tracking deformation.
                if (tri.dm_inv.isZero()) {
                    ModelBuilder::compute_triangle_pose(
                        mb.particle_q[it0->second],
                        mb.particle_q[it1->second],
                        mb.particle_q[it2->second],
                        tri.dm_inv, tri.area);
                }

                mb.add_triangle(it0->second, it1->second, it2->second,
                                tri.dm_inv, tri.area,
                                tri.mu, tri.lambda, tri.thickness);
            });

        // hinges: explicit entities take priority, auto-generate from triangle adjacency if none
        int hinge_count = 0;
        world.query_builder<Hinge>()
            .build()
            .each([&](flecs::entity, Hinge& hinge) {
                auto it0 = entity_to_index.find(hinge.v0.id());
                auto it1 = entity_to_index.find(hinge.v1.id());
                auto it2 = entity_to_index.find(hinge.v2.id());
                auto it3 = entity_to_index.find(hinge.v3.id());
                if (it0 == entity_to_index.end() ||
                    it1 == entity_to_index.end() ||
                    it2 == entity_to_index.end() ||
                    it3 == entity_to_index.end()) return;

                if (hinge.rest_angle == 0) {
                    bending::Eval e;
                    if (bending::eval(
                            mb.particle_q[it0->second],
                            mb.particle_q[it1->second],
                            mb.particle_q[it2->second],
                            mb.particle_q[it3->second], e)) {
                        hinge.rest_angle = e.angle;
                    }
                }

                mb.add_edge(it0->second, it1->second,
                            it2->second, it3->second,
                            hinge.rest_angle, hinge.stiffness, 0);
                hinge_count++;
            });

        // auto-generate from triangle adjacency if no explicit hinges
        if (hinge_count == 0 && !mb.tri_indices.empty()) {
            const Real k_bend = default_bending_stiffness;
            auto edges = build_adjacency(mb.tri_indices.data(),
                                         (int)mb.tri_indices.size() / 3);
            for (const auto& e : edges) {
                if (e.f1 == -1) continue; // skip boundary edges

                bending::Eval ev;
                Real rest = 0;
                if (bending::eval(mb.particle_q[e.o0], mb.particle_q[e.o1],
                                  mb.particle_q[e.v0], mb.particle_q[e.v1], ev)) {
                    rest = ev.angle;
                }

                mb.add_edge(e.o0, e.o1, e.v0, e.v1, rest, k_bend, 0);
            }
        }

        return mb.finalize();
    }

    // ECS -> State
    void gather(State& state) const {
        for (int i = 0; i < (int)particle_entities.size(); i++) {
            state.q(i) = particle_entities[i].get<Position>().map();
            state.qd(i) = particle_entities[i].get<Velocity>().map();
        }
    }

    // State -> ECS
    void scatter(const State& state) const {
        for (int i = 0; i < (int)particle_entities.size(); i++) {
            particle_entities[i].ensure<Position>().map() = state.q(i);
            particle_entities[i].ensure<Velocity>().map() = state.qd(i);
        }
    }
};

} // namespace physics
