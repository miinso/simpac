#include "broad.h"
#include "types.h"

// #include <_types/_uint64_t.h>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <cassert>
// #include "../util.h"


std::vector<Broad_Collision_Pair> broad_get_collision_pairs(const std::vector<flecs::entity>& entities) {
    std::vector<Broad_Collision_Pair> collision_pairs;
    collision_pairs.reserve(32);

    for (size_t i = 0; i < entities.size(); ++i) {
        auto e1 = entities[i];
        for (size_t j = i + 1; j < entities.size(); ++j) {
            // Entity* e2 = entities[j];
            auto e2 = entities[j];

            auto x1 = e1.get<phys::pbd::Position>()->value;
            auto x2 = e2.get<phys::pbd::Position>()->value;

            auto radius1 = e1.get<phys::pbd::BoundingSphere>()->value;
            auto radius2 = e1.get<phys::pbd::BoundingSphere>()->value;

            Real entities_distance = (x1 - x2).norm();

            // Increase the distance a little to account for moving objects.
            // @TODO: We should derivate this value from delta_time, forces, velocities, etc
            // Real max_distance_for_collision = e1->bounding_sphere_radius + e2->bounding_sphere_radius + 0.1;
            Real max_distance_for_collision = radius1 + radius2 + 0.1;
            if (entities_distance <= max_distance_for_collision) {
                Broad_Collision_Pair pair;
                pair.e1 = e1;
                pair.e2 = e2;
                collision_pairs.push_back(pair);
            }
        }
    }

    return collision_pairs;
}

// Union-Find implementation
namespace {
    // Find with path compression
    flecs::entity_t uf_find(std::unordered_map<flecs::entity_t, flecs::entity_t>& entity_to_parent_map, flecs::entity_t x) {
        auto it = entity_to_parent_map.find(x);
        assert(it != entity_to_parent_map.end());
        flecs::entity_t p = it->second;
        
        if (p == x) {
            return x;
        }

        // Path compression
        flecs::entity_t root = uf_find(entity_to_parent_map, p);
        entity_to_parent_map[x] = root;
        return root;
    }

    // Union by pointing one root to another
    void uf_union(std::unordered_map<flecs::entity_t, flecs::entity_t>& entity_to_parent_map, flecs::entity_t x, flecs::entity_t y) {
        flecs::entity_t root_x = uf_find(entity_to_parent_map, x);
        flecs::entity_t root_y = uf_find(entity_to_parent_map, y);
        
        if (root_x != root_y) {
            entity_to_parent_map[root_y] = root_x;
        }
    }

    // Collect all entities into a union-find structure
    std::unordered_map<flecs::entity_t, flecs::entity_t> uf_collect_all(
        const std::vector<flecs::entity>& entities, 
        const std::vector<Broad_Collision_Pair>& collision_pairs) {
        
        std::unordered_map<flecs::entity_t, flecs::entity_t> entity_to_parent_map;
        
        // Initialize each entity to point to itself
        for (const auto& entity : entities) {
            entity_to_parent_map[entity] = entity;
        }

        // Union entities in collision pairs
        for (const auto& collision_pair : collision_pairs) {
            auto e1 = collision_pair.e1;
            auto e2 = collision_pair.e2;
            
            // Get entity objects
            // Entity* e1 = entity_get_by_id(id1);
            // Entity* e2 = entity_get_by_id(id2);
            // e1.id();
            
            // TODO: call this function with active entities only to avoid `e1.has<phys::pbd::IsPinned>()`
            
            // if (!e1->fixed && !e2->fixed) {
            if (!e1.has<phys::pbd::IsPinned>() && !e2.has<phys::pbd::IsPinned>()) {
                uf_union(entity_to_parent_map, e1.id(), e2.id());
            }
        }

        return entity_to_parent_map;
    }
}

std::vector<std::vector<flecs::entity>> broad_collect_simulation_islands(
    const std::vector<flecs::entity>& entities, 
    const std::vector<Broad_Collision_Pair>& collision_pairs, 
    const std::vector<phys::pbd::Constraint>* constraints) {
    
    std::vector<std::vector<flecs::entity>> simulation_islands;
    simulation_islands.reserve(32);

    // Collect the simulation islands into an entity->parent map
    auto entity_to_parent_map = uf_collect_all(entities, collision_pairs);

    // Extra step: To avoid bugs, we need to make sure that entities that are part of a same constraint are also part of the same island!
    if (constraints != nullptr) { // contact constraints
        for (const auto& c : *constraints) {
            // flecs::entity id1 = c.e1_id;
            // flecs::entity id2 = c.e2_id;
            auto e1 = c.e1;
            auto e2 = c.e2;
            
            // Entity* e1 = entity_get_by_id(id1);
            // Entity* e2 = entity_get_by_id(id2);

            // if (!e1->fixed && !e2->fixed) {
            if (!e1.has<phys::pbd::IsPinned>() && !e2.has<phys::pbd::IsPinned>()) {
                uf_union(entity_to_parent_map, e1, e2);
            }
        }
    }

    // As a last step, transform the simulation islands into a nice structure
    std::unordered_map<flecs::entity_t, size_t> simulation_islands_map;

    for (const auto& entity : entities) {
        if (entity.has<phys::pbd::IsPinned>()) {
            continue;
        }
        
        flecs::entity_t parent = uf_find(entity_to_parent_map, entity);
        
        auto it = simulation_islands_map.find(parent);
        if (it == simulation_islands_map.end()) {
            // Simulation Island not created yet.
            std::vector<flecs::entity> new_simulation_island;
            new_simulation_island.reserve(32);
            
            size_t simulation_island_idx = simulation_islands.size();
            simulation_islands.push_back(new_simulation_island);
            simulation_islands_map[parent] = simulation_island_idx;
            
            // Add the entity to the new island
            simulation_islands.back().push_back(entity);
        } else {
            // Add to existing island
            simulation_islands[it->second].push_back(entity);
        }
    }

    return simulation_islands;
}

void broad_simulation_islands_destroy(std::vector<std::vector<flecs::entity>>& simulation_islands) {
    // With std::vector, we don't need explicit cleanup
    simulation_islands.clear();
}