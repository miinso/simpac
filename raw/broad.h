#ifndef RAW_PHYSICS_PHYSICS_BROAD_H
#define RAW_PHYSICS_PHYSICS_BROAD_H

#include <vector>
// #include "../render/graphics.h"
// #include "pbd.h"
#include "types.h"
#include <flecs.h>

struct Broad_Collision_Pair {
    // TODO: to hold `flecs::entity` or `flecs::entity.id()`?
    flecs::entity e1;
    flecs::entity e2;
};

std::vector<Broad_Collision_Pair> broad_get_collision_pairs(const std::vector<flecs::entity>& entities);
std::vector<std::vector<flecs::entity>> broad_collect_simulation_islands(
    const std::vector<flecs::entity>& entities, 
    const std::vector<Broad_Collision_Pair>& collision_pairs, 
    const std::vector<phys::pbd::Constraint>* constraints = nullptr);
void broad_simulation_islands_destroy(std::vector<std::vector<flecs::entity>>& simulation_islands);

#endif