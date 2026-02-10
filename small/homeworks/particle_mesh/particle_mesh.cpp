// Particle Mesh Simulator
// Parent system pattern with explicit .run() calls

#include "components.h"
#include "systems.h"
#include "utils.h"
#include "common/components.h"
#include "common/conversions.h"
#include "common/geometry.h"
#include "common/random.h"
#include "graphics.h"
#include "flecs.h"
#include <chrono>
#include <iostream>

using namespace particle_mesh;

int main() {
    std::cout << "Hi from " << __FILE__ << std::endl;

    flecs::world ecs;

    // Register components
    ecs.component<Position>();
    ecs.component<Velocity>();
    ecs.component<ParticleProperties>();
    ecs.component<PlaneProperties>();
    ecs.component<Particle>();
    ecs.component<Plane>();
    ecs.component<Scene>().add(flecs::Singleton);
    ecs.component<SimulationResources>().add(flecs::Singleton);

    ecs.set<Scene>({
        -9.8f * Eigen::Vector3f::UnitY(),
        0.5f,
        1.0f / 100.0f,
        0.0f
    });

    // Initialize graphics
    graphics::init(ecs);
    graphics::init_window(CANVAS_WIDTH, CANVAS_HEIGHT, "Particle Mesh Simulator");

    // Initialize simulation resources
    SimulationResources res;
    res.sphere_model = LoadModelFromMesh(GenMeshSphere(1, 4, 4));
    ecs.set<SimulationResources>(res);

    // Create initial particle
    create_particle(ecs, {1, 5, 0}, {5, -10, 10});

    // Create generator
    DiskGenerator diskGenerator{
        Eigen::Vector3f(0, 5, 0),
        Eigen::Vector3f(0, -1, 0),
        0.5f,
        2000.0f,
        0.0f
    };
    ecs.entity("DiskGenerator").set<DiskGenerator>(diskGenerator);

    // Install bodies
    install_bodies(ecs);

    // =========================================================================
    // Simulation systems (no .kind() - called manually via .run())
    // =========================================================================

    auto handle_keypress = ecs.system("HandleKeypress")
        .kind(0)
        .run(systems::handle_keypress);

    auto update_spatial_hash = ecs.system("UpdateSpatialHash")
        .kind(0)
        .run(systems::update_spatial_hash);

    auto update_disk_generator = ecs.system<DiskGenerator>("UpdateDiskGenerator")
        .kind(0)
        .each([](DiskGenerator& generator) {
            const auto& camera = graphics::get_raylib_camera_const();
            generator.center = toEig(camera.position);
            generator.normal = toEig(camera.target) - toEig(camera.position);
            generator.normal.normalize();
        });

    auto generate_particles = ecs.system<DiskGenerator>("GenerateParticles")
        .kind(0)
        .each(systems::generateParticles);

    auto integrate_particles = ecs.system<Position, Velocity, particle_lifespan, const ParticleProperties, const Scene>("IntegrateParticles")
        .kind(0)
        .each([](flecs::iter& it, size_t index, Position& p, Velocity& v, particle_lifespan& pl,
                 const ParticleProperties& props, const Scene& scene) {
            if (!props.is_rest) {
                v.value += it.delta_time() * (1.0f / props.mass) *
                           (scene.gravity + scene.air_resistance * -v.value);
                p.value += it.delta_time() * v.value;
            }
            pl.t += it.delta_time();
            if (pl.t > props.lifespan) {
                it.entity(index).destruct();
            }
        });

    auto particle_collision = ecs.system<Position, Velocity, ParticleProperties>("ParticleTriangleCollision")
        .kind(0)
        .each(systems::particle_triangle_collision);

    // =========================================================================
    // Rendering systems (on graphics phases - run via ecs.progress())
    // =========================================================================

    ecs.system<const Position, const ParticleProperties>("DrawParticles")
        .kind(graphics::OnRender)
        .each(systems::draw_particles);

    ecs.system<const body, const trimesh>("render_body")
        .kind(graphics::OnRender)
        .each([](const body& b, const trimesh& t) { systems::render_body(b, t); });

    ecs.system<DiskGenerator>("render_generator")
        .kind(graphics::OnRender)
        .each([](const DiskGenerator& gen) { systems::render_generator(gen); });

    ecs.system<const aabb, const body>("RenderAABB")
        .kind(graphics::OnRender)
        .each(systems::render_aabb);

    ecs.system<Scene>("DisplayTimingInfo")
        .kind(graphics::PostRender)
        .each([](flecs::iter& it, size_t, Scene& scene) {
            scene.frame_time = GetFrameTime() * 1000.0;
            auto particle_query = it.world().query<const Particle>();
            int total_particles = particle_query.count();
            DrawText(TextFormat("Simulation Took: %.2f ms", scene.sim_time), 20, 60, 20, LIME);
            DrawText(TextFormat("Frame Took: %.2f ms", scene.frame_time), 20, 80, 20, LIME);
            DrawText(TextFormat("Total Particles: %d", total_particles), 20, 120, 20, LIME);
        });

    // Fixed 60 Hz simulation tick (decoupled from display rate)
    auto sim_tick = ecs.timer().interval(1.0f / 60.0f);

    ecs.system("ParticleSimulation")
        .kind(flecs::PreUpdate)
        .tick_source(sim_tick)
        .run([&](flecs::iter& it) {
            float dt = it.delta_time();
            auto& scene = it.world().get_mut<Scene>();

            auto start_time = std::chrono::high_resolution_clock::now();

            // Input
            handle_keypress.run(dt);

            // Generator update
            update_disk_generator.run(dt);
            generate_particles.run(dt);

            // Collision detection setup
            update_spatial_hash.run(dt);

            // Physics
            integrate_particles.run(dt);
            particle_collision.run(dt);

            auto end_time = std::chrono::high_resolution_clock::now();
            scene.sim_time = std::chrono::duration<double, std::milli>(end_time - start_time).count();
            scene.accumulated_time += dt;
        });

    // Main loop
    graphics::run_loop();

    std::cout << "Simulation ended." << std::endl;
    return 0;
}
