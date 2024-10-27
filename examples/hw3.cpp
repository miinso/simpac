#include "Eigen/Dense"
#include "Eigen/Geometry"
#include "flecs.h"
#include "graphics_module.h"
#include <array>
#include <chrono>
#include <iostream>
#include <random>
#include <unordered_map>
#include <vector>

#include "hw2aux.h"

constexpr int CANVAS_WIDTH = 800;
constexpr int CANVAS_HEIGHT = 600;
constexpr float EPSILON = 1e-6f;

// components
struct Position {
    Eigen::Vector3f value;
};

struct Velocity {
    Eigen::Vector3f value;
};

struct Acceleration {
    Eigen::Vector3f value;
};

struct Boid { };
struct Predator { };

struct BoidProps {
    Eigen::Vector3f position;
    Eigen::Vector3f velocity;
    Eigen::Vector3f acceleration;
    float max_speed;
    float max_force;
    float size;
    Eigen::Vector3f color;
};

struct Obstacle {
    Eigen::Vector3f position;
    float radius;
};

struct FlockingParams {
    float separation_distance;
    float alignment_distance;
    float cohesion_distance;
    float separation_weight;
    float alignment_weight;
    float cohesion_weight;
    float obstacle_avoidance_radius;
    float obstacle_avoidance_weight;
    float predator_avoidance_radius;
    float predator_avoidance_weight;
};

struct PredatorFlockingParams {
    float separation_distance;
    float alignment_distance;
    float cohesion_distance;
    float separation_weight;
    float alignment_weight;
    float cohesion_weight;
    float obstacle_avoidance_radius;
    float obstacle_avoidance_weight;
    float prey_attraction_weight;
    float prey_attraction_radius;
};

struct Scene {
    Eigen::Vector3f gravity;
    float air_resistance;
    float simulation_time_step;
    float accumulated_time;

    double frame_time;
    double sim_time;

    Eigen::Vector3f min_bounds;
    Eigen::Vector3f max_bounds;
};

// hashing
struct SpatialHash {
    float spacing;
    int tableSize;
    std::vector<int> cellStart;
    std::vector<int> cellEntries;
    std::vector<int> queryIds;
    int querySize;

    SpatialHash(float spacing, int maxNumObjects)
        : spacing(spacing)
        , tableSize(2 * maxNumObjects)
        , querySize(0) {
        cellStart.resize(tableSize + 1, 0);
        cellEntries.resize(maxNumObjects, 0);
        queryIds.resize(maxNumObjects, 0);
    }

    int hashCoords(int xi, int yi, int zi) const {
        unsigned int h = (xi * 92837111) ^ (yi * 689287499) ^ (zi * 283923481);
        return h % tableSize;
    }

    int intCoord(float coord) const {
        return static_cast<int>(std::floor(coord / spacing));
    }

    int hashPos(const Eigen::Vector3f& pos) const {
        return hashCoords(intCoord(pos.x()), intCoord(pos.y()), intCoord(pos.z()));
    }

    void create(const std::vector<Eigen::Vector3f>& positions, const Eigen::Vector3f& min_bounds) {
        int numObjects = std::min(static_cast<int>(positions.size()), static_cast<int>(cellEntries.size()));

        // Determine cell sizes
        std::fill(cellStart.begin(), cellStart.end(), 0);
        std::fill(cellEntries.begin(), cellEntries.end(), 0);

        for (int i = 0; i < numObjects; ++i) {
            int h = hashPos(positions[i]);
            ++cellStart[h];
        }

        // Determine cell starts
        int start = 0;
        for (int i = 0; i < numObjects; ++i) {
            Eigen::Vector3f adjusted_pos = positions[i] - min_bounds;
            int h = hashPos(adjusted_pos);
            ++cellStart[h];
        }
        cellStart[tableSize] = start; // guard

        // Fill in object ids
        for (int i = 0; i < numObjects; ++i) {
            Eigen::Vector3f adjusted_pos = positions[i] - min_bounds;
            int h = hashPos(adjusted_pos);
            --cellStart[h];
            cellEntries[cellStart[h]] = i;
        }
    }

    void query(const Eigen::Vector3f& pos, float maxDist) {
        int x0 = intCoord(pos.x() - maxDist);
        int y0 = intCoord(pos.y() - maxDist);
        int z0 = intCoord(pos.z() - maxDist);

        int x1 = intCoord(pos.x() + maxDist);
        int y1 = intCoord(pos.y() + maxDist);
        int z1 = intCoord(pos.z() + maxDist);

        querySize = 0;

        for (int xi = x0; xi <= x1; ++xi) {
            for (int yi = y0; yi <= y1; ++yi) {
                for (int zi = z0; zi <= z1; ++zi) {
                    int h = hashCoords(xi, yi, zi);
                    int start = cellStart[h];
                    int end = cellStart[h + 1];

                    for (int i = start; i < end; ++i) {
                        queryIds[querySize++] = cellEntries[i];
                    }
                }
            }
        }
    }
};

// Component to hold the SpatialHash
struct SpatialHashComponent {
    std::unique_ptr<SpatialHash> hash;
};

// System to update the spatial hash
void update_spatial_hash(flecs::iter& it) {
    auto* spatial_hash = it.world().get_mut<SpatialHashComponent>();
    const auto& scene = *it.world().get<Scene>();
    
    if (!spatial_hash->hash) {
        // Initialize the spatial hash if it doesn't exist
        float spacing = 0.5f; // Adjust this based on your needs
        int maxNumObjects = 10; // Assuming all entities with BoidProps are relevant
        spatial_hash->hash = std::make_unique<SpatialHash>(spacing, maxNumObjects);
    }

    std::vector<Eigen::Vector3f> positions;
    positions.reserve(10);

    // for (auto i : it) {
    //     positions.push_back(boids[i].position);
    // }

    it.world().each([&](BoidProps& boid) { positions.push_back(boid.position); });

    spatial_hash->hash->create(positions, scene.min_bounds);
}

void visualize_spatial_hash(flecs::iter& it) {
    auto* spatial_hash = it.world().get<SpatialHashComponent>();
    if (!spatial_hash || !spatial_hash->hash)
        return;

    const auto& hash = *spatial_hash->hash;
    const auto& scene = *it.world().get<Scene>();

    Eigen::Vector3f min_bounds = scene.min_bounds;
    Eigen::Vector3f max_bounds = scene.max_bounds;

    for (int x = hash.intCoord(min_bounds.x()); x <= hash.intCoord(max_bounds.x()); ++x) {
        for (int y = hash.intCoord(min_bounds.y()); y <= hash.intCoord(max_bounds.y()); ++y) {
            for (int z = hash.intCoord(min_bounds.z()); z <= hash.intCoord(max_bounds.z()); ++z) {
                int h = hash.hashCoords(x, y, z);
                int count = hash.cellStart[h + 1] - hash.cellStart[h];

                if (count > 0) {
                    Eigen::Vector3f cell_min(x * hash.spacing, y * hash.spacing, z * hash.spacing);
                    Eigen::Vector3f cell_max = cell_min + Eigen::Vector3f::Constant(hash.spacing);

                    Color color = count >= 50 ? RED : (count >= 10 ? ORANGE : GREEN);

                    DrawBoundingBox({e2r(cell_min), e2r(cell_max)}, color);
                }
            }
        }
    }
}

// callables
Eigen::Vector3f random_vector() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(-1.0, 1.0);
    return Eigen::Vector3f(dis(gen), dis(gen), dis(gen)).normalized();
}

template <typename ParamType>
Eigen::Vector3f separate(const BoidProps& boid, const std::vector<BoidProps>& neighbors, const ParamType& params) {
    Eigen::Vector3f steer = Eigen::Vector3f::Zero();
    int count = 0;

    for (const auto& neighbor : neighbors) {
        float d = (boid.position - neighbor.position).norm();
        if (d > 0 && d < params.separation_distance) {
            Eigen::Vector3f diff = (boid.position - neighbor.position).normalized() / d;
            steer += diff;
            count++;
        }
    }

    if (count > 0) {
        steer /= count;
        steer.normalize();
        steer *= boid.max_speed;
        steer -= boid.velocity;
        steer = steer.cwiseMin(boid.max_force).cwiseMax(-boid.max_force);
    }
    return steer;
}

template <typename ParamType>
Eigen::Vector3f align(const BoidProps& boid, const std::vector<BoidProps>& neighbors, const ParamType& params) {
    Eigen::Vector3f sum = Eigen::Vector3f::Zero();
    int count = 0;

    for (const auto& neighbor : neighbors) {
        float d = (boid.position - neighbor.position).norm();
        if (d > 0 && d < params.alignment_distance) {
            sum += neighbor.velocity;
            count++;
        }
    }

    if (count > 0) {
        sum /= count;
        sum.normalize();
        sum *= boid.max_speed;
        Eigen::Vector3f steer = sum - boid.velocity;
        return steer.cwiseMin(boid.max_force).cwiseMax(-boid.max_force);
    }
    return Eigen::Vector3f::Zero();
}

template <typename ParamType>
Eigen::Vector3f cohesion(const BoidProps& boid, const std::vector<BoidProps>& neighbors, const ParamType& params) {
    Eigen::Vector3f sum = Eigen::Vector3f::Zero();
    int count = 0;

    for (const auto& neighbor : neighbors) {
        float d = (boid.position - neighbor.position).norm();
        if (d > 0 && d < params.cohesion_distance) {
            sum += neighbor.position;
            count++;
        }
    }

    if (count > 0) {
        sum /= count;
        Eigen::Vector3f desired = sum - boid.position;
        desired.normalize();
        desired *= boid.max_speed;
        Eigen::Vector3f steer = desired - boid.velocity;
        return steer.cwiseMin(boid.max_force).cwiseMax(-boid.max_force);
    }
    return Eigen::Vector3f::Zero();
}

Eigen::Vector3f avoid_obstacles(
    const BoidProps& boid, const std::vector<Obstacle>& obstacles, float avoidance_radius, float avoidance_weight) {
    Eigen::Vector3f avoidance_force = Eigen::Vector3f::Zero();
    for (const auto& obstacle : obstacles) {
        Eigen::Vector3f to_obstacle = obstacle.position - boid.position;
        float distance = to_obstacle.norm();

        if (distance < avoidance_radius + obstacle.radius) {
            Eigen::Vector3f avoidance = -to_obstacle.normalized() * (avoidance_radius + obstacle.radius - distance);
            avoidance_force += avoidance;
        }
    }
    return avoidance_force * avoidance_weight;
}

Eigen::Vector3f avoid_predators(
    const BoidProps& boid, const std::vector<BoidProps>& predators, float avoidance_radius, float avoidance_weight) {
    Eigen::Vector3f avoidance_force = Eigen::Vector3f::Zero();
    for (const auto& predator : predators) {
        Eigen::Vector3f to_predator = predator.position - boid.position;
        float distance = to_predator.norm();

        if (distance < avoidance_radius) {
            Eigen::Vector3f avoidance = -to_predator.normalized() * (avoidance_radius - distance);
            avoidance_force += avoidance;
        }
    }
    return avoidance_force * avoidance_weight;
}

Eigen::Vector3f attract_to_prey(
    const BoidProps& predator, const std::vector<BoidProps>& prey, float attraction_radius, float attraction_weight) {
    Eigen::Vector3f attraction_force = Eigen::Vector3f::Zero();
    int count = 0;

    for (const auto& boid : prey) {
        Eigen::Vector3f to_prey = boid.position - predator.position;
        float distance = to_prey.norm();

        if (distance < attraction_radius) {
            attraction_force += to_prey.normalized();
            count++;
        }
    }

    if (count > 0) {
        attraction_force /= count;
        attraction_force *= attraction_weight;
    }

    return attraction_force;
}

// systems
void apply_flocking_behavior(flecs::entity e, BoidProps& boid) {
    auto params = e.world().get<FlockingParams>();
    std::vector<BoidProps> neighbors;
    std::vector<BoidProps> predators;
    std::vector<Obstacle> obstacles;

    // Collect neighbors and predators
    e.world().each([&](flecs::entity other_e, Boid, BoidProps& other) {
        if (other_e != e && (boid.position - other.position).norm() < params->cohesion_distance) {
            neighbors.push_back(other);
        }
    });

    e.world().each([&](Predator, BoidProps& predator) { predators.push_back(predator); });
    e.world().each([&](const Obstacle& obstacle) { obstacles.push_back(obstacle); });

    boid.acceleration.setZero();

    Eigen::Vector3f separation = separate(boid, neighbors, *params) * params->separation_weight;
    Eigen::Vector3f alignment = align(boid, neighbors, *params) * params->alignment_weight;
    Eigen::Vector3f cohesion2 = cohesion(boid, neighbors, *params) * params->cohesion_weight;
    Eigen::Vector3f predator_avoidance =
        avoid_predators(boid, predators, params->predator_avoidance_radius, params->predator_avoidance_weight);
    Eigen::Vector3f obstacle_avoidance =
        avoid_obstacles(boid, obstacles, params->obstacle_avoidance_radius, params->obstacle_avoidance_weight);

    boid.acceleration += separation + alignment + cohesion2 + predator_avoidance + obstacle_avoidance;
}

void apply_predator_flocking_behavior(flecs::entity e, BoidProps& predator) {
    auto params = e.world().get<PredatorFlockingParams>();
    std::vector<BoidProps> neighbors;
    std::vector<BoidProps> prey;
    std::vector<Obstacle> obstacles;

    // Collect predator neighbors and prey
    e.world().each([&](flecs::entity other_e, Predator, BoidProps& other) {
        if (other_e != e && (predator.position - other.position).norm() < params->cohesion_distance) {
            neighbors.push_back(other);
        }
    });

    e.world().each([&](Boid, BoidProps& boid) { prey.push_back(boid); });
    e.world().each([&](const Obstacle& obstacle) { obstacles.push_back(obstacle); });

    // return;

    predator.acceleration.setZero();

    Eigen::Vector3f separation = separate(predator, neighbors, *params) * params->separation_weight;
    Eigen::Vector3f alignment = align(predator, neighbors, *params) * params->alignment_weight;
    Eigen::Vector3f cohesion2 = cohesion(predator, neighbors, *params) * params->cohesion_weight;
    Eigen::Vector3f prey_attraction =
        attract_to_prey(predator, prey, params->prey_attraction_radius, params->prey_attraction_weight);
    Eigen::Vector3f obstacle_avoidance =
        avoid_obstacles(predator, obstacles, params->obstacle_avoidance_radius, params->obstacle_avoidance_weight);

    predator.acceleration += separation + alignment + cohesion2 + prey_attraction + obstacle_avoidance;
}

Eigen::Vector3f steer_to_avoid_bounds(
    const Eigen::Vector3f& position, const Eigen::Vector3f& velocity, float max_speed, const Scene& scene) {

    const float margin = 0.5f; // Distance from edge to start steering
    const float turn_factor = 1.0f; // Strength of steering
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

void update_boid_movement(flecs::iter& it, size_t, BoidProps& boid) {
    float dt = it.delta_time();
    auto scene = it.world().get_mut<Scene>();

    Eigen::Vector3f boundary_force = steer_to_avoid_bounds(boid.position, boid.velocity, boid.max_speed, *scene);
    boid.acceleration += boundary_force;

    boid.velocity += boid.acceleration * dt;
    boid.velocity = boid.velocity.normalized() * std::min(boid.velocity.norm(), boid.max_speed);
    boid.position += boid.velocity * dt;

    boid.position = boid.position.cwiseMax(scene->min_bounds).cwiseMin(scene->max_bounds);

    // boid.acceleration.setZero();
}

void draw_boid(flecs::iter& it, size_t index, const BoidProps& boid) {
    const auto color =
        Color{static_cast<unsigned char>(boid.color.x() * 255),
              static_cast<unsigned char>(boid.color.y() * 255),
              static_cast<unsigned char>(boid.color.z() * 255),
              255};

    if (it.entity(index).has<Boid>()) {
        DrawCylinderWiresEx(
            e2r(boid.position),
            e2r(boid.position + boid.velocity.normalized() * boid.size * 3.0f),
            1.0f * boid.size,
            0.1f * boid.size,
            8,
            color);
    } else if (it.entity(index).has<Predator>()) {
        DrawCylinderEx(e2r(boid.position),
                       e2r(boid.position + boid.velocity.normalized() * boid.size * 3.0f),
                       1.0f * boid.size,
                       0.1f * boid.size,
                       8,
                       color);
    }
}

void draw_obstacle(flecs::iter& it, size_t, const Obstacle& obstacle) {
    DrawSphere(e2r(obstacle.position), obstacle.radius, Color{150, 75, 0, 255});
}

void draw_aabb(flecs::iter& it) {
    auto scene = it.world().get_mut<Scene>();
    DrawBoundingBox({e2r(scene->min_bounds), e2r(scene->max_bounds)}, BLUE);
}

void handle_keypress_system(flecs::iter& it) {
    const auto& camera =
        graphics::graphics::activeCamera == 1 ? graphics::graphics::camera1 : graphics::graphics::camera2;
    const auto origin = r2e(camera.position);
    const auto target = r2e(camera.target);
    const auto velocity = (target - origin + Eigen::Vector3f::UnitY() * 2.0f).normalized() * 2.0f;

    if (IsKeyPressed(KEY_R)) {
        it.world().each([&](flecs::entity e, const Boid&) { e.destruct(); });
    }
}

int main() {

    std::cout << "Hi from " << __FILE__ << std::endl;

    flecs::world ecs;

    ecs.component<Boid>();
    ecs.component<BoidProps>();
    ecs.component<Position>();
    ecs.component<Velocity>();
    ecs.component<Acceleration>();

    ecs.set<FlockingParams>(
        {0.3f, // separation_distance
         0.3f, // alignment_distance
         1.0f, // cohesion_distance
         1.5f, // separation_weight
         1.0f, // alignment_weight
         1.0f, // cohesion_weight
         2.0f,
         3.0f,
         2.0f,
         7.0f});

    ecs.set<PredatorFlockingParams>(
        {3.0f, // separation_distance
         5.0f, // alignment_distance
         5.0f, // cohesion_distance
         1.0f, // separation_weight
         1.0f, // alignment_weight
         0.0f, // cohesion_weight
         0.5f, // prey_attraction_weight
         1.0f, // prey_attraction_radius
         1.0f,
         2.0f});

    ecs.set<Scene>({
        -9.8f * Eigen::Vector3f::UnitY(), // gravity
        0.5f, // air_resistance
        1.0f / 100.0f, // simulation_time_step
        0.0f, // accumulated_time
        0.0, // frame_time (initialized to 0)
        0.0, // sim_time (initialized to 0)
        // Eigen::Vector3f(-5.0f, -2.0f, -2.0f), // min_bounds
        // Eigen::Vector3f(5.0f, 2.0f, 2.0f) // max_bounds
        Eigen::Vector3f(0.0f, 0.0f, 0.0f), // min_bounds
        Eigen::Vector3f(5.0f, 2.0f, 5.0f) // max_bounds
    });

    ecs.component<SpatialHashComponent>();
    ecs.set<SpatialHashComponent>({});

    // Import and initialize graphics
    ecs.import <graphics::graphics>();
    graphics::graphics::init_window(ecs, CANVAS_WIDTH, CANVAS_HEIGHT, "Boid Simulator");

    for (int i = 0; i < 10; i++) {
        ecs.entity().add<Boid>().set<BoidProps>({
            Eigen::Vector3f::Random() * 1, // position
            random_vector() * 2, // velocity
            Eigen::Vector3f::Zero(), // acceleration
            5.0f, // max_speed
            2.0f, // max_force
            0.1f, // size
            Eigen::Vector3f::Random().cwiseAbs() // color
        });
    }

    // for (int i = 0; i < 3; i++) {
    //     ecs.entity().add<Predator>().set<BoidProps>({
    //         Eigen::Vector3f::Random() * 2, // position
    //         random_vector() * 1.5, // velocity
    //         Eigen::Vector3f::Zero(), // acceleration
    //         1.0f, // max_speed
    //         0.5f, // max_force
    //         0.5f, // size
    //         Eigen::Vector3f(0.2f, 0.2f, 0.2f) // color (red-ish)
    //     });
    // }

    ecs.entity().set<Obstacle>({Eigen::Vector3f(1, 0, 0), 1.0f});

    ecs.system("HandleKeypress").kind(flecs::PreUpdate).run(handle_keypress_system);

    ecs.system("UpdateSpatialHash").kind(flecs::PreUpdate).run(update_spatial_hash);
    ecs.system<BoidProps>("ApplyFlockingBehavior").kind(flecs::PreUpdate).with<Boid>().each(apply_flocking_behavior);
    ecs.system<BoidProps>("ApplyPredatorFlockingBehavior")
        .kind(flecs::PreUpdate)
        .with<Predator>()
        .each(apply_predator_flocking_behavior);

    ecs.system<BoidProps>("UpdateBoidMovement").kind(flecs::OnUpdate).each(update_boid_movement);

    // post update
    ecs.system("graphics::begin").kind(flecs::PostUpdate).run([](flecs::iter& it) {
        it.world().get_mut<Scene>()->frame_time = 0.0;
        BeginDrawing();
    });
    ecs.system("graphics::clear").kind(flecs::PostUpdate).run([](flecs::iter& it) { ClearBackground(RAYWHITE); });
    ecs.system("graphics::begin_mode_3d").kind(flecs::PostUpdate).run([](flecs::iter& it) {
        BeginMode3D(graphics::graphics::activeCamera == 1 ? graphics::graphics::camera1 : graphics::graphics::camera2);
        DrawGrid(12, 10.0f / 12);
    });
    ecs.system<const BoidProps>("DrawBoids").kind(flecs::PostUpdate).each(draw_boid);
    ecs.system<const Obstacle>("DrawObstacles").kind(flecs::PostUpdate).each(draw_obstacle);
    ecs.system("DrawSceneExtent").kind(flecs::PostUpdate).run(draw_aabb);
    ecs.system("VisualizeSpatialHash").kind(flecs::PostUpdate).run(visualize_spatial_hash);

    ecs.system("graphics::end_mode_3d").kind(flecs::OnStore).run([](flecs::iter& it) {
        EndMode3D();
        DrawFPS(20, 20);
    });

    ecs.system("DisplayTimingInfo").kind(flecs::OnStore).run([](flecs::iter& it) {
        auto scene = it.world().get_mut<Scene>();
        scene->frame_time = GetFrameTime() * 1000.0; // to milliseconds

        auto boid_query = it.world().query<const Boid>();
        int total_boids = boid_query.count();

        DrawText(TextFormat("Simulation Took: %.2f ms", scene->sim_time), 20, 60, 20, LIME);
        DrawText(TextFormat("Frame Took: %.2f ms", scene->frame_time), 20, 80, 20, LIME);
        DrawText(TextFormat("Total Boids: %d", total_boids), 20, 120, 20, LIME);
    });

    ecs.system("graphics::end").kind(flecs::OnStore).run([](flecs::iter& it) { EndDrawing(); });

    // main loop
    graphics::graphics::run_main_loop(ecs, []() {
        // callback if needed
    });

    std::cout << "Simulation ended." << std::endl;
    return 0;
}