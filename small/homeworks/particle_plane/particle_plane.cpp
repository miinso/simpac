// Particle Plane Simulator
// Parent system pattern with substep accumulator

#include "components.h"
#include "systems.h"
#include "utils.h"
#include "common/components.h"
#include "common/conversions.h"
#include "graphics.h"
#include "flecs.h"
#include <chrono>
#include <iostream>

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

using namespace particle_plane;

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
    ecs.component<AudioState>().add(flecs::Singleton);

    ecs.set<Scene>({
        -9.8f * Eigen::Vector3f::UnitY(),
        0.5f,
        1.0f / 1000.0f,
        0.0f
    });

    // Initialize graphics
    graphics::init(ecs);
    graphics::init_window(CANVAS_WIDTH, CANVAS_HEIGHT, "Particle Plane Simulator");
    InitAudioDevice();

    // Initialize simulation resources
    SimulationResources res;
    Image checked = GenImageChecked(6, 6, 1, 1, LIGHTGRAY, DARKGRAY);
    res.texture = LoadTextureFromImage(checked);
    UnloadImage(checked);

    res.plane_model = LoadModelFromMesh(GenMeshPlane(2, 2, 4, 3));
    res.plane_model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = res.texture;

    Mesh cubeMesh = GenMeshCube(2, 0.0f, 2);
    GenMeshTangents(&cubeMesh);
    res.cube_model = LoadModelFromMesh(cubeMesh);
    res.cube_model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = res.texture;

    res.sphere_model = LoadModelFromMesh(GenMeshSphere(1, 64, 64));
    res.sphere_model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = res.texture;
    ecs.set<SimulationResources>(res);

    // Initialize audio
    AudioState audio;
    audio.notes[0] = LoadSound("resources/note0.ogg");
    audio.notes[1] = LoadSound("resources/note1.ogg");
    audio.notes[2] = LoadSound("resources/note2.ogg");
    audio.notes[3] = LoadSound("resources/note3.ogg");
    audio.notes[4] = LoadSound("resources/note4.ogg");
    audio.sequence[0] = &audio.notes[1];
    audio.sequence[1] = &audio.notes[2];
    audio.sequence[2] = &audio.notes[3];
    audio.sequence[3] = &audio.notes[2];
    audio.sequence[4] = &audio.notes[1];
    audio.sequence[5] = &audio.notes[4];
    audio.sequence[6] = &audio.notes[4];
    audio.sequence[7] = &audio.notes[4];
    audio.sequence[8] = nullptr;
    audio.collision_sfx_count = 0;
    ecs.set<AudioState>(audio);

    // Create initial entities
    create_particle(ecs, {1, 5, 0}, {5, -10, 10});
    create_plane(ecs, {5, 5, 0}, {-1, 0, 0});
    create_plane(ecs, {-5, 5, 0}, {1, 0, 0});
    create_plane(ecs, {0, 0, 0}, {0, 1, 0}, 0.3f, 0.7f, false, false);  // ground
    create_plane(ecs, {0, 15, 0}, {0, -1, 0}, 0.3f, 0.7f, false, true); // ceiling
    create_plane(ecs, {0, 5, 5}, {0, 0, -1});
    create_plane(ecs, {0, 5, -5}, {0, 0, 1});

    // =========================================================================
    // Simulation systems (no .kind() - called manually via .run())
    // =========================================================================

    auto create_particle_on_input = ecs.system("CreateParticleOnInput")
        .kind(0)
        .run(systems::create_particle_on_input);

    auto reset_particles = ecs.system("ResetParticles")
        .kind(0)
        .run(systems::reset_particles);

    auto simulate_particles = ecs.system<Position, Velocity, const ParticleProperties>("SimulateParticles")
        .kind(0)
        .each([](flecs::iter& it, size_t index, Position& p, Velocity& v, const ParticleProperties& props) {
            const auto& scene = it.world().get<Scene>();
            if (!props.is_rest) {
                v.value += it.delta_time() * (1.0f / props.mass) * (scene.gravity + scene.air_resistance * -v.value);
                p.value += it.delta_time() * v.value;
            }
        });

    auto handle_collisions = ecs.system<Position, Velocity, ParticleProperties>("HandleCollisions")
        .kind(0)
        .each([](flecs::iter& it, size_t index, Position& p, Velocity& v, ParticleProperties& props) {
            systems::handle_particle_plane_collisions(it, index, p, v, props);
            systems::handle_particle_particle_collisions(it, index, p, v, props);
        });

    auto check_rest_state = ecs.system<Position, Velocity, ParticleProperties>("CheckRestState")
        .kind(0)
        .each([](flecs::iter& it, size_t index, Position& p, Velocity& v, ParticleProperties& props) {
            systems::check_particle_rest_state(it, index, p, v, props);
        });

    // =========================================================================
    // Rendering systems (on graphics phases - run via ecs.progress())
    // =========================================================================

    ecs.system<const Position, const ParticleProperties>("DrawParticles")
        .kind(graphics::phase_on_render)
        .each(systems::draw_particles);

    ecs.system<const Position, const PlaneProperties>("DrawPlanes")
        .kind(graphics::phase_on_render)
        .each(systems::draw_planes);

    ecs.system<Scene>("DisplayTimingInfo")
        .kind(graphics::phase_post_render)
        .each([](flecs::iter& it, size_t, Scene& scene) {
            scene.frame_time = GetFrameTime() * 1000.0;
            DrawText(TextFormat("Simulation Took: %.2f ms", scene.sim_time), 20, 60, 20, LIME);
            DrawText(TextFormat("Frame Took: %.2f ms", scene.frame_time), 20, 80, 20, LIME);
            DrawText(TextFormat("Simulation Timestep: %.7f ms", scene.simulation_time_step), 20, 120, 20, LIME);
        });

    ecs.system<Scene>("HandleTimestepControl")
        .kind(graphics::phase_post_render)
        .each([](flecs::iter& it, size_t, Scene& scene) {
            float logValue = systems::LinearToLog(scene.frequency_slider, scene.min_frequency, scene.max_frequency);

            const float adjustment_factor = 0.2f;
            if (IsKeyPressed(KEY_PERIOD)) {
                logValue = fmin(logValue + adjustment_factor, 1.0f);
            }
            if (IsKeyPressed(KEY_COMMA)) {
                logValue = fmax(logValue - adjustment_factor, 0.0f);
            }

            GuiSlider(Rectangle{20, 140, 200, 20}, nullptr, nullptr, &logValue, 0.0f, 1.0f);
            scene.frequency_slider = systems::LogToLinear(logValue, scene.min_frequency, scene.max_frequency);
            scene.simulation_time_step = 1.0f / scene.frequency_slider;

            float airvalue = scene.air_resistance;
            DrawText(TextFormat("Air Resistance: %.2f", scene.air_resistance), 20, 180, 20, LIME);
            GuiSlider(Rectangle{20, 200, 200, 20}, nullptr, nullptr, &airvalue, 0.0f, 1.0f);
            scene.air_resistance = airvalue;
        });

    // =========================================================================
    // Parent system - THE ALGORITHM in one place
    // =========================================================================

    // Fixed 60 Hz tick source
    auto sim_tick = ecs.timer().interval(1.0f / 60.0f);

    ecs.system("ParticleSimulation")
        .kind(flecs::PreUpdate)
        .tick_source(sim_tick)
        .run([&](flecs::iter& it) {
            float dt = it.delta_time();
            auto& scene = it.world().get_mut<Scene>();

            auto start_time = std::chrono::high_resolution_clock::now();

            // Accumulate time and run substeps
            scene.accumulated_time += dt;
            while (scene.accumulated_time >= scene.simulation_time_step) {
                float substep_dt = scene.simulation_time_step;

                // Input
                create_particle_on_input.run(substep_dt);
                reset_particles.run(substep_dt);

                // Physics
                simulate_particles.run(substep_dt);
                handle_collisions.run(substep_dt);
                check_rest_state.run(substep_dt);

                scene.accumulated_time -= scene.simulation_time_step;
            }

            auto end_time = std::chrono::high_resolution_clock::now();
            scene.sim_time = std::chrono::duration<double, std::milli>(end_time - start_time).count();
        });

    // Main loop
    graphics::run_loop();

    std::cout << "Simulation ended." << std::endl;
    return 0;
}
