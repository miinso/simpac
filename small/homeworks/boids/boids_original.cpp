// Boid Simulator - Original Pipeline Version (for benchmarking)
// Systems registered directly on phases, no parent system orchestration

#include "components.h"
#include "systems.h"
#include "utils.h"
#include "common/components.h"
#include "common/random.h"
#include "graphics.h"
#include "flecs.h"
#include <iostream>

using namespace boids;

int main() {
    std::cout << "Hi from " << __FILE__ << " (Original Pipeline Version)" << std::endl;

    flecs::world ecs;

    // Register components
    ecs.component<Boid>();
    ecs.component<Predator>();
    ecs.component<Position>();
    ecs.component<Velocity>();
    ecs.component<Acceleration>();
    ecs.component<MaxSpeed>();
    ecs.component<MaxForce>();
    ecs.component<Size>();
    ecs.component<BoidColor>();
    ecs.component<NearbyBoids>();
    ecs.component<BoidSystems>();
    ecs.component<SpatialHashComponent>();
    ecs.component<DebugSettings>();

    // Boid flocking params
    ecs.set<FlockingParams, Boid>({
        0.2f,  // separation_distance
        0.2f,  // alignment_distance
        0.2f,  // cohesion_distance
        1.5f,  // separation_weight
        1.0f,  // alignment_weight
        1.0f,  // cohesion_weight
        2.0f,  // obstacle_avoidance_radius
        3.0f,  // obstacle_avoidance_weight
        2.0f,  // predator_avoidance_radius
        7.0f,  // predator_avoidance_weight
        0.0f,  // prey_attraction_radius (unused for boids)
        0.0f   // prey_attraction_weight (unused for boids)
    });

    // Predator flocking params
    ecs.set<FlockingParams, Predator>({
        3.0f,  // separation_distance
        5.0f,  // alignment_distance
        5.0f,  // cohesion_distance
        1.0f,  // separation_weight
        1.0f,  // alignment_weight
        0.0f,  // cohesion_weight
        1.0f,  // obstacle_avoidance_radius
        2.0f,  // obstacle_avoidance_weight
        0.0f,  // predator_avoidance_radius (unused for predators)
        0.0f,  // predator_avoidance_weight (unused for predators)
        0.5f,  // prey_attraction_radius
        1.0f   // prey_attraction_weight
    });

    // Scene setup
    ecs.set<Scene>({
        -9.8f * Eigen::Vector3f::UnitY(),
        0.5f,
        1.0f / 100.0f,
        0.0f,
        0.0,
        0.0,
        Eigen::Vector3f(-2.0f, -2.0f, -5.0f),
        Eigen::Vector3f(2.0f, 2.0f, 5.0f)
    });

    ecs.set<SpatialHashComponent>({});
    ecs.set<DebugSettings>({});

    // Cache queries
    BoidQueries queries;
    queries.boids = ecs.query<const Position, Boid>();
    queries.predators = ecs.query<const Position, Predator>();
    queries.obstacles = ecs.query<const Obstacle>();
    ecs.set<BoidQueries>(queries);

    // Init spatial hash (runs once at start via OnStart phase)
    ecs.system("InitSpatialHash").kind(flecs::OnStart).run([](flecs::iter& it) {
        auto& spatial_hash = it.world().get_mut<SpatialHashComponent>();
        const auto& params = it.world().get<FlockingParams, Boid>();
        float spacing = params.cohesion_distance * 1.0f;
        int maxNumObjects = 5000;
        int tableSize = maxNumObjects * 2;
        spatial_hash.hash.init(spacing, maxNumObjects, tableSize);
    });

    // Init graphics
    graphics::init(ecs);
    graphics::init_window(CANVAS_WIDTH, CANVAS_HEIGHT, "Boid Simulator (Original)");

    // Create initial boids
    const auto& scene_bounds = ecs.get<Scene>();
    Eigen::Vector3f extent = scene_bounds.max_bounds - scene_bounds.min_bounds;
    for (int i = 0; i < 500; i++) {
        Eigen::Vector3f pos = scene_bounds.min_bounds +
            (Eigen::Vector3f::Random() * 0.5f + Eigen::Vector3f::Constant(0.5f)).cwiseProduct(extent);
        create_boid(ecs, pos, randomVector() * 2);
    }

    // Create obstacle
    create_obstacle(ecs, Eigen::Vector3f(1, 0, 0), 1.0f);

    // =========================================================================
    // Fixed 60 Hz simulation tick (same as parent system version)
    // =========================================================================
    auto sim_tick = ecs.timer().interval(1.0f / 60.0f);

    // =========================================================================
    // Simulation systems - registered on PreUpdate phase with tick_source
    // =========================================================================

    ecs.system("HandleKeypress")
        .kind(flecs::PreUpdate)
        .tick_source(sim_tick)
        .run(systems::handle_keypress);

    auto update_hash = ecs.system("UpdateSpatialHash")
        .kind(flecs::PreUpdate)
        .tick_source(sim_tick)
        .run(systems::update_spatial_hash);

    auto find_spatial = ecs.system<const Position, NearbyBoids>("FindNeighborsSpatial")
        .kind(flecs::PreUpdate)
        .tick_source(sim_tick)
        .with<Boid>()
        .each(systems::find_neighbors_spatial);

    auto find_brute = ecs.system<const Position, NearbyBoids>("FindNeighborsBrute")
        .kind(flecs::PreUpdate)
        .tick_source(sim_tick)
        .with<Boid>()
        .each(systems::find_neighbors_brute);

    // Start with brute force (spatial hash disabled)
    update_hash.disable();
    find_spatial.disable();

    ecs.set<BoidSystems>({update_hash, find_spatial, find_brute});

    // Reset acceleration
    ecs.system<Acceleration>("ResetAcceleration")
        .kind(flecs::PreUpdate)
        .tick_source(sim_tick)
        .with<Boid>()
        .each([](Acceleration& acc) { acc.value.setZero(); });

    // Flocking systems
    ecs.system<const Position, const Velocity, const MaxSpeed, const MaxForce, Acceleration, const NearbyBoids>("Separation")
        .kind(flecs::PreUpdate)
        .tick_source(sim_tick)
        .with<Boid>()
        .each([](flecs::entity e, const Position& pos, const Velocity& vel, const MaxSpeed& max_speed,
                 const MaxForce& max_force, Acceleration& acc, const NearbyBoids& nearby) {
            const auto& params = e.world().get<FlockingParams, Boid>();
            systems::separation(pos, vel, max_speed, max_force, acc, nearby, params);
        });

    ecs.system<const Position, const Velocity, const MaxSpeed, const MaxForce, Acceleration, const NearbyBoids>("Alignment")
        .kind(flecs::PreUpdate)
        .tick_source(sim_tick)
        .with<Boid>()
        .each([](flecs::entity e, const Position& pos, const Velocity& vel, const MaxSpeed& max_speed,
                 const MaxForce& max_force, Acceleration& acc, const NearbyBoids& nearby) {
            const auto& params = e.world().get<FlockingParams, Boid>();
            systems::alignment(pos, vel, max_speed, max_force, acc, nearby, params);
        });

    ecs.system<const Position, const Velocity, const MaxSpeed, const MaxForce, Acceleration, const NearbyBoids>("Cohesion")
        .kind(flecs::PreUpdate)
        .tick_source(sim_tick)
        .with<Boid>()
        .each([](flecs::entity e, const Position& pos, const Velocity& vel, const MaxSpeed& max_speed,
                 const MaxForce& max_force, Acceleration& acc, const NearbyBoids& nearby) {
            const auto& params = e.world().get<FlockingParams, Boid>();
            systems::cohesion(pos, vel, max_speed, max_force, acc, nearby, params);
        });

    ecs.system<const Position, Acceleration, const NearbyBoids>("PredatorAvoidance")
        .kind(flecs::PreUpdate)
        .tick_source(sim_tick)
        .with<Boid>()
        .each([](flecs::entity e, const Position& pos, Acceleration& acc, const NearbyBoids& nearby) {
            const auto& params = e.world().get<FlockingParams, Boid>();
            systems::predator_avoidance(pos, acc, nearby, params);
        });

    ecs.system<const Position, Acceleration, const NearbyBoids>("ObstacleAvoidance")
        .kind(flecs::PreUpdate)
        .tick_source(sim_tick)
        .with<Boid>()
        .each([](flecs::entity e, const Position& pos, Acceleration& acc, const NearbyBoids& nearby) {
            const auto& params = e.world().get<FlockingParams, Boid>();
            systems::obstacle_avoidance(pos, acc, nearby, params);
        });

    // Predator flocking
    ecs.system<const Position, const Velocity, const MaxSpeed, const MaxForce, Acceleration>("PredatorFlocking")
        .kind(flecs::PreUpdate)
        .tick_source(sim_tick)
        .with<Predator>()
        .each([](flecs::entity e, const Position& pos, const Velocity& vel, const MaxSpeed& max_speed,
                 const MaxForce& max_force, Acceleration& acc) {
            const auto& params = e.world().get<FlockingParams, Predator>();
            std::vector<flecs::entity> neighbors;
            std::vector<flecs::entity> prey;
            std::vector<flecs::entity> obstacles;

            e.world().each([&](flecs::entity other_e, Predator, const Position& other_pos) {
                if (other_e != e && (pos.value - other_pos.value).norm() < params.cohesion_distance) {
                    neighbors.push_back(other_e);
                }
            });

            e.world().each([&](flecs::entity be, Boid, const Position&) { prey.push_back(be); });
            e.world().each([&](flecs::entity oe, const Obstacle&) { obstacles.push_back(oe); });

            acc.value.setZero();

            NearbyBoids nearby;
            nearby.neighbors = neighbors;
            nearby.obstacles = obstacles;

            systems::separation(pos, vel, max_speed, max_force, acc, nearby, params);
            systems::alignment(pos, vel, max_speed, max_force, acc, nearby, params);
            systems::cohesion(pos, vel, max_speed, max_force, acc, nearby, params);
            systems::prey_attraction(pos, acc, prey, params);
            systems::obstacle_avoidance(pos, acc, nearby, params);
        });

    // Movement update
    ecs.system<Position, Velocity, Acceleration, const MaxSpeed>("UpdateMovement")
        .kind(flecs::OnUpdate)
        .tick_source(sim_tick)
        .each(systems::update_movement);

    // =========================================================================
    // Rendering systems (on graphics phases - run every frame, no tick_source)
    // =========================================================================

    ecs.system<const Position, const Velocity, const Size, const BoidColor>("DrawBoids")
        .kind(graphics::phase_on_render)
        .each(systems::draw_boid);

    ecs.system<const Obstacle>("DrawObstacles")
        .kind(graphics::phase_on_render)
        .each([](const Obstacle& obstacle) { systems::draw_obstacle(obstacle); });

    ecs.system("DrawBounds")
        .kind(graphics::phase_on_render)
        .run(systems::draw_bounds);

    ecs.system("VisualizeSpatialHash")
        .kind(graphics::phase_on_render)
        .run(systems::visualize_spatial_hash);

    ecs.system("DisplayTimingInfo")
        .kind(graphics::phase_post_render)
        .run([](flecs::iter& it) {
            auto& scene = it.world().get_mut<Scene>();
            scene.frame_time = GetFrameTime() * 1000.0;

            auto boid_query = it.world().query<const Boid>();
            int total_boids = boid_query.count();

            const auto& debug = it.world().get<DebugSettings>();

            DrawText(TextFormat("Simulation Took: %.2f ms", scene.sim_time), 20, 60, 20, LIME);
            DrawText(TextFormat("Frame Took: %.2f ms", scene.frame_time), 20, 80, 20, LIME);
            DrawText(TextFormat("Total Boids: %d", total_boids), 20, 120, 20, LIME);
            DrawText(TextFormat("Spatial Hash: %s (H to toggle)", debug.use_spatial_hash ? "ON" : "OFF"), 20, 140, 20,
                     debug.use_spatial_hash ? GREEN : RED);
            DrawText(TextFormat("Hash Viz: %s (V to toggle)", debug.show_spatial_hash ? "ON" : "OFF"), 20, 160, 20,
                     debug.show_spatial_hash ? GREEN : GRAY);
        });

    // Main loop - all systems run via pipeline
    graphics::run_loop();

    std::cout << "Simulation ended." << std::endl;
    return 0;
}
