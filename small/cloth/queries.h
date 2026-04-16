#pragma once

#include "components.h"
#include <flecs.h>

namespace queries {

inline flecs::query<const Position> particle_query;
inline flecs::query<const Spring> spring_query;
inline flecs::query<const Triangle> triangle_query;
inline flecs::query<const Position, const Mass, ParticleState> particle_pick_query;

// build cached query handles once, after component registration.
inline void seed(flecs::world& ecs) {
    particle_query = ecs.query_builder<const Position>()
        .with<Particle>()
        .cached()
        .build();

    spring_query = ecs.query_builder<const Spring>()
        .cached()
        .build();

    triangle_query = ecs.query_builder<const Triangle>()
        .cached()
        .build();

    particle_pick_query = ecs.query_builder<const Position, const Mass, ParticleState>()
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
