#pragma once

#include "components.h"
#include "common/components.h"
#include "common/conversions.h"
#include "common/random.h"
#include "common/spatial_hash.h"
#include "Eigen/Dense"
#include "flecs.h"
#include <raylib.h>
#include <unordered_map>
#include <algorithm>

namespace boids::systems {

// Helper: limit steering force
inline Eigen::Vector3f limit_steering(
    const Eigen::Vector3f& steer,
    const Eigen::Vector3f& velocity,
    float max_speed,
    float max_force) {
    if (steer.norm() < 1e-6f) return Eigen::Vector3f::Zero();
    Eigen::Vector3f desired = steer.normalized() * max_speed;
    Eigen::Vector3f result = desired - velocity;
    return result.cwiseMin(max_force).cwiseMax(-max_force);
}

// Separation: steer away from nearby boids
inline void separation(
    const Position& pos,
    const Velocity& vel,
    const MaxSpeed& max_speed,
    const MaxForce& max_force,
    Acceleration& acc,
    const NearbyBoids& nearby,
    const FlockingParams& params) {

    Eigen::Vector3f steer = Eigen::Vector3f::Zero();
    int count = 0;

    for (const auto& neighbor : nearby.neighbors) {
        const auto& other_pos = neighbor.get<Position>().value;
        float d = (pos.value - other_pos).norm();
        if (d > 0 && d < params.separation_distance) {
            steer += (pos.value - other_pos).normalized() / d;
            count++;
        }
    }

    if (count > 0) {
        steer /= count;
        acc.value += limit_steering(steer, vel.value, max_speed.value, max_force.value)
                   * params.separation_weight;
    }
}

// Alignment: steer toward average heading of neighbors
inline void alignment(
    const Position& pos,
    const Velocity& vel,
    const MaxSpeed& max_speed,
    const MaxForce& max_force,
    Acceleration& acc,
    const NearbyBoids& nearby,
    const FlockingParams& params) {

    Eigen::Vector3f sum = Eigen::Vector3f::Zero();
    int count = 0;

    for (const auto& neighbor : nearby.neighbors) {
        const auto& other_pos = neighbor.get<Position>().value;
        const auto& other_vel = neighbor.get<Velocity>().value;
        float d = (pos.value - other_pos).norm();
        if (d > 0 && d < params.alignment_distance) {
            sum += other_vel;
            count++;
        }
    }

    if (count > 0) {
        sum /= count;
        acc.value += limit_steering(sum, vel.value, max_speed.value, max_force.value)
                   * params.alignment_weight;
    }
}

// Cohesion: steer toward center of neighbors
inline void cohesion(
    const Position& pos,
    const Velocity& vel,
    const MaxSpeed& max_speed,
    const MaxForce& max_force,
    Acceleration& acc,
    const NearbyBoids& nearby,
    const FlockingParams& params) {

    Eigen::Vector3f sum = Eigen::Vector3f::Zero();
    int count = 0;

    for (const auto& neighbor : nearby.neighbors) {
        const auto& other_pos = neighbor.get<Position>().value;
        float d = (pos.value - other_pos).norm();
        if (d > 0 && d < params.cohesion_distance) {
            sum += other_pos;
            count++;
        }
    }

    if (count > 0) {
        sum /= count;
        acc.value += limit_steering(sum - pos.value, vel.value, max_speed.value, max_force.value)
                   * params.cohesion_weight;
    }
}

// Obstacle avoidance
inline void obstacle_avoidance(
    const Position& pos,
    Acceleration& acc,
    const NearbyBoids& nearby,
    const FlockingParams& params) {

    for (const auto& obstacle_e : nearby.obstacles) {
        const auto& obstacle = obstacle_e.get<Obstacle>();
        Eigen::Vector3f to_obstacle = obstacle.position - pos.value;
        float distance = to_obstacle.norm();

        if (distance < params.obstacle_avoidance_radius + obstacle.radius) {
            Eigen::Vector3f avoidance = -to_obstacle.normalized()
                * (params.obstacle_avoidance_radius + obstacle.radius - distance);
            acc.value += avoidance * params.obstacle_avoidance_weight;
        }
    }
}

// Predator avoidance
inline void predator_avoidance(
    const Position& pos,
    Acceleration& acc,
    const NearbyBoids& nearby,
    const FlockingParams& params) {

    for (const auto& predator_e : nearby.predators) {
        const auto& predator_pos = predator_e.get<Position>().value;
        Eigen::Vector3f to_predator = predator_pos - pos.value;
        float distance = to_predator.norm();

        if (distance < params.predator_avoidance_radius) {
            Eigen::Vector3f avoidance = -to_predator.normalized()
                * (params.predator_avoidance_radius - distance);
            acc.value += avoidance * params.predator_avoidance_weight;
        }
    }
}

// Prey attraction (for predators)
inline void prey_attraction(
    const Position& pos,
    Acceleration& acc,
    const std::vector<flecs::entity>& prey,
    const FlockingParams& params) {

    Eigen::Vector3f attraction_force = Eigen::Vector3f::Zero();
    int count = 0;

    for (const auto& prey_e : prey) {
        const auto& prey_pos = prey_e.get<Position>().value;
        Eigen::Vector3f to_prey = prey_pos - pos.value;
        float distance = to_prey.norm();

        if (distance < params.prey_attraction_radius) {
            attraction_force += to_prey.normalized();
            count++;
        }
    }

    if (count > 0) {
        attraction_force /= count;
        acc.value += attraction_force * params.prey_attraction_weight;
    }
}

// Boundary steering
inline Eigen::Vector3f steer_to_avoid_bounds(
    const Eigen::Vector3f& position,
    const Eigen::Vector3f& velocity,
    float max_speed,
    const Scene& scene) {

    const float margin = 0.5f;
    const float turn_factor = 1.0f;
    Eigen::Vector3f steering(0, 0, 0);

    for (int i = 0; i < 3; ++i) {
        if (position[i] < scene.min_bounds[i] + margin) {
            steering[i] = turn_factor * ((scene.min_bounds[i] + margin) - position[i]);
        } else if (position[i] > scene.max_bounds[i] - margin) {
            steering[i] = turn_factor * ((scene.max_bounds[i] - margin) - position[i]);
        }
    }

    if (steering.norm() > 0) {
        steering = steering.normalized() * max_speed - velocity;
        steering = steering.cwiseMin(max_speed).cwiseMax(-max_speed);
    }

    return steering;
}

// Spatial hash update
inline void update_spatial_hash(flecs::iter& it) {
    auto& spatial_hash = it.world().get_mut<SpatialHashComponent>();
    spatial_hash.entities.clear();

    std::vector<Eigen::Vector3f> positions;
    it.world().each([&](flecs::entity e, Boid, const Position& pos) {
        spatial_hash.entities.push_back(e);
        positions.push_back(pos.value);
    });

    spatial_hash.hash.create(positions);
}

// Visualize spatial hash
inline void visualize_spatial_hash(flecs::iter& it) {
    const auto& debug = it.world().get<DebugSettings>();
    if (!debug.show_spatial_hash) return;

    const auto& spatial_hash = it.world().get<SpatialHashComponent>();
    const auto& hash = spatial_hash.hash;
    if (!hash.initialized) return;

    std::unordered_map<int64_t, int> occupied_cells;
    std::unordered_map<int, std::vector<int64_t>> bucket_to_cells;

    for (const auto& entity : spatial_hash.entities) {
        const auto& pos = entity.get<Position>().value;
        int cx = hash.intCoord(pos.x());
        int cy = hash.intCoord(pos.y());
        int cz = hash.intCoord(pos.z());
        int64_t key = (int64_t(cx) & 0xFFFFF) | ((int64_t(cy) & 0xFFFFF) << 20) | ((int64_t(cz) & 0xFFFFF) << 40);

        if (occupied_cells.find(key) == occupied_cells.end()) {
            int bucket = hash.hashPos(pos);
            bucket_to_cells[bucket].push_back(key);
        }
        occupied_cells[key]++;
    }

    int collision_count = 0;
    int total_cells = 0;

    for (const auto& [cell_key, boid_count] : occupied_cells) {
        ++total_cells;

        int cx = static_cast<int>(cell_key & 0xFFFFF);
        int cy = static_cast<int>((cell_key >> 20) & 0xFFFFF);
        int cz = static_cast<int>((cell_key >> 40) & 0xFFFFF);
        if (cx >= 0x80000) cx -= 0x100000;
        if (cy >= 0x80000) cy -= 0x100000;
        if (cz >= 0x80000) cz -= 0x100000;

        int bucket = hash.hashCoords(cx, cy, cz);
        auto bucket_it = bucket_to_cells.find(bucket);
        bool has_collision = bucket_it != bucket_to_cells.end() && bucket_it->second.size() > 1;

        if (has_collision) ++collision_count;

        Eigen::Vector3f cell_min(cx * hash.spacing, cy * hash.spacing, cz * hash.spacing);
        Eigen::Vector3f cell_max = cell_min + Eigen::Vector3f::Constant(hash.spacing);

        Color color;
        if (has_collision) {
            color = MAGENTA;
        } else if (boid_count >= 50) {
            color = RED;
        } else if (boid_count >= 10) {
            color = ORANGE;
        } else {
            color = GREEN;
        }
        DrawBoundingBox({toRay(cell_min), toRay(cell_max)}, color);
    }

    DrawText(TextFormat("Hash: %d cells, %d collisions (%.1f%%)",
        total_cells, collision_count, total_cells > 0 ? 100.0f * collision_count / total_cells : 0.0f),
        10, 80, 16, WHITE);
}

// Neighbor finding - spatial hash
inline void find_neighbors_spatial(flecs::entity e, const Position& pos, NearbyBoids& nearby) {
    const auto& params = e.world().get<FlockingParams, Boid>();
    auto& spatial_hash = e.world().get_mut<SpatialHashComponent>();
    const auto& queries = e.world().get<BoidQueries>();

    nearby.neighbors.clear();
    nearby.predators.clear();
    nearby.obstacles.clear();

    float query_radius = std::max({params.separation_distance, params.alignment_distance, params.cohesion_distance});
    spatial_hash.hash.query(pos.value, query_radius);

    for (int i = 0; i < spatial_hash.hash.querySize; ++i) {
        int idx = spatial_hash.hash.queryIds[i];
        if (idx >= 0 && idx < static_cast<int>(spatial_hash.entities.size()) && spatial_hash.entities[idx] != e) {
            flecs::entity other = spatial_hash.entities[idx];
            const auto& other_pos = other.get<Position>().value;
            if ((pos.value - other_pos).norm() < params.cohesion_distance) {
                nearby.neighbors.push_back(other);
            }
        }
    }

    queries.predators.each([&](flecs::entity pe, const Position&, Predator) { nearby.predators.push_back(pe); });
    queries.obstacles.each([&](flecs::entity oe, const Obstacle&) { nearby.obstacles.push_back(oe); });
}

// Neighbor finding - brute force
inline void find_neighbors_brute(flecs::entity e, const Position& pos, NearbyBoids& nearby) {
    const auto& params = e.world().get<FlockingParams, Boid>();
    const auto& queries = e.world().get<BoidQueries>();

    nearby.neighbors.clear();
    nearby.predators.clear();
    nearby.obstacles.clear();

    queries.boids.each([&](flecs::entity other_e, const Position& other_pos, Boid) {
        if (other_e != e && (pos.value - other_pos.value).norm() < params.cohesion_distance) {
            nearby.neighbors.push_back(other_e);
        }
    });

    queries.predators.each([&](flecs::entity pe, const Position&, Predator) { nearby.predators.push_back(pe); });
    queries.obstacles.each([&](flecs::entity oe, const Obstacle&) { nearby.obstacles.push_back(oe); });
}

// Movement update
inline void update_movement(flecs::iter& it, size_t, Position& pos, Velocity& vel, Acceleration& acc, const MaxSpeed& max_speed) {
    float dt = it.delta_time();
    auto& scene = it.world().get_mut<Scene>();

    Eigen::Vector3f boundary_force = steer_to_avoid_bounds(pos.value, vel.value, max_speed.value, scene);
    acc.value += boundary_force;

    vel.value += acc.value * dt;
    vel.value = vel.value.normalized() * std::min(vel.value.norm(), max_speed.value);
    pos.value += vel.value * dt;

    pos.value = pos.value.cwiseMax(scene.min_bounds).cwiseMin(scene.max_bounds);
}

// Draw boid
inline void draw_boid(flecs::iter& it, size_t index, const Position& pos, const Velocity& vel, const Size& size, const BoidColor& col) {
    const auto color =
        Color{static_cast<unsigned char>(col.value.x() * 255),
              static_cast<unsigned char>(col.value.y() * 255),
              static_cast<unsigned char>(col.value.z() * 255),
              255};

    Eigen::Vector3f tip = pos.value + vel.value.normalized() * size.value * 3.0f;

    if (it.entity(index).has<Boid>()) {
        DrawCylinderWiresEx(
            toRay(pos.value),
            toRay(tip),
            1.0f * size.value,
            0.1f * size.value,
            8,
            color);
    } else if (it.entity(index).has<Predator>()) {
        DrawCylinderEx(toRay(pos.value),
                       toRay(tip),
                       1.0f * size.value,
                       0.1f * size.value,
                       8,
                       color);
    }
}

// Draw obstacle
inline void draw_obstacle(const Obstacle& obstacle) {
    DrawSphere(toRay(obstacle.position), obstacle.radius, Color{150, 75, 0, 255});
}

// Draw scene bounds
inline void draw_bounds(flecs::iter& it) {
    const auto& scene = it.world().get<Scene>();
    DrawBoundingBox({toRay(scene.min_bounds), toRay(scene.max_bounds)}, BLUE);
}

// Handle keypress
inline void handle_keypress(flecs::iter& it) {
    auto& debug = it.world().get_mut<DebugSettings>();

    if (IsKeyPressed(KEY_R)) {
        it.world().each([&](flecs::entity e, const Boid&) { e.destruct(); });
    }

    if (IsKeyPressed(KEY_EQUAL) || IsKeyPressed(KEY_KP_ADD)) {
        const auto& scene = it.world().get<Scene>();
        Eigen::Vector3f extent = scene.max_bounds - scene.min_bounds;
        for (int i = 0; i < 500; i++) {
            Eigen::Vector3f pos = scene.min_bounds +
                (Eigen::Vector3f::Random() * 0.5f + Eigen::Vector3f::Constant(0.5f)).cwiseProduct(extent);
            it.world().entity()
                .add<Boid>()
                .set<Position>({pos})
                .set<Velocity>({randomVector() * 2})
                .set<Acceleration>({Eigen::Vector3f::Zero()})
                .set<MaxSpeed>({5.0f})
                .set<MaxForce>({2.0f})
                .set<Size>({0.1f})
                .set<BoidColor>({Eigen::Vector3f::Random().cwiseAbs()})
                .set<NearbyBoids>({});
        }
    }

    if (IsKeyPressed(KEY_MINUS) || IsKeyPressed(KEY_KP_SUBTRACT)) {
        std::vector<flecs::entity> boids;
        it.world().each([&](flecs::entity e, const Boid&) { boids.push_back(e); });

        int to_remove = std::min(500, static_cast<int>(boids.size()));
        for (int i = 0; i < to_remove; i++) {
            int idx = rand() % boids.size();
            boids[idx].destruct();
            boids[idx] = boids.back();
            boids.pop_back();
        }
    }

    if (IsKeyPressed(KEY_H)) {
        debug.use_spatial_hash = !debug.use_spatial_hash;

        // For pipeline version: toggle systems via BoidSystems component
        if (it.world().has<BoidSystems>()) {
            const auto& sys = it.world().get<BoidSystems>();
            if (debug.use_spatial_hash) {
                sys.update_hash.enable();
                sys.find_neighbors_spatial.enable();
                sys.find_neighbors_brute.disable();
            } else {
                sys.update_hash.disable();
                sys.find_neighbors_spatial.disable();
                sys.find_neighbors_brute.enable();
            }
        }
        // For parent system version: uses if/else in parent lambda
    }

    if (IsKeyPressed(KEY_V)) {
        debug.show_spatial_hash = !debug.show_spatial_hash;
    }
}

} // namespace boids::systems
