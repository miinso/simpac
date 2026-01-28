#pragma once

#include "components.h"
#include <flecs.h>

namespace queries {

inline flecs::query<const Position> particle_query;
inline flecs::query<const Spring> spring_query;
inline flecs::query<const Position, ParticleState> particle_pick_query;

inline void init(flecs::world& ecs) {
    particle_query = ecs.query_builder<const Position>()
        .with<Particle>()
        .cached()
        .build();

    spring_query = ecs.query_builder<const Spring>()
        .cached()
        .build();

    particle_pick_query = ecs.query_builder<const Position, ParticleState>()
        .with<Particle>()
        .cached()
        .build();
}

inline int num_particles() {
    return (int)particle_query.count();
}

inline int num_springs() {
    return (int)spring_query.count();
}

} // namespace queries
