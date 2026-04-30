#pragma once

#include "model.h"
#include "../components/particle.h"
#include "../components/constraint.h"
#include <unordered_map>

namespace physics {

// maps ECS entities <-> solver indices, handles gather/scatter
struct Bridge {
    std::vector<flecs::entity> particle_entities; // [particle_count]
    std::unordered_map<uint64_t, int> entity_to_index; // internal, for resolving Spring/Triangle refs

    // build Model from current ECS world state
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
                auto it_a = entity_to_index.find(spring.e1.id());
                auto it_b = entity_to_index.find(spring.e2.id());
                if (it_a == entity_to_index.end() || it_b == entity_to_index.end()) return;

                mb.add_spring(it_a->second, it_b->second,
                              spring.k_s, spring.k_d, spring.rest_length);
            });

        world.query_builder<Triangle>()
            .build()
            .each([&](flecs::entity, Triangle& tri) {
                auto it0 = entity_to_index.find(tri.e1.id());
                auto it1 = entity_to_index.find(tri.e2.id());
                auto it2 = entity_to_index.find(tri.e3.id());
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
