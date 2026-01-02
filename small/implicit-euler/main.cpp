// Implicit Euler Mass-Spring Simulator

#include "components.h"
#include "systems.h"
#include "utils.h"
#include "flecs.h"
#include "graphics.h"
#include <cstdio>


// =========================================================================
// Main
// =========================================================================

int main() {
    printf("Hi from %s\n", __FILE__);

    flecs::world ecs;

    // Scene setup (timestep = 0.01)
    Vector3r gravity(0, -9.81, 0);
    ecs.component<Scene>().add(flecs::Singleton);
    ecs.set<Scene>({
        0.01,   // timestep
        100,    // num_substeps (not used)
        10,     // solve_iter
        gravity,
        0.0     // elapsed
    });

    ecs.component<Solver>().add(flecs::Singleton);
    // ecs.set<Solver>();
    auto& solver = ecs.ensure<Solver>();
    solver.b.setZero();
    solver.A.setZero();

    // Register SpringRenderer as singleton component
    ecs.component<SpringRenderer>().add(flecs::Singleton);
    auto& gpu = ecs.ensure<SpringRenderer>();

    // Initialize graphics
    graphics::init(ecs);
    graphics::init_window(800, 600, "Implicit Euler");
    graphics::init_camera({
        {50.0f, 10.0f, 50.0f}, // position
        {0.0f, 0.5f, 0.0f}, // target
    });

    // =========================================================================
    // Simulation systems (we set .kind(0) to manually run them
    // =========================================================================

    auto collect_external_force = ecs.system<const Mass, const Velocity, const ParticleIndex, Solver>("Collect External Force")
        .with<Particle>()
        .kind(0)
        .each([&](flecs::iter& it, size_t, const Mass& m, const Velocity& v, const ParticleIndex& idx, Solver& solver) {
            systems::collect_external_force(m, v, idx, gravity, it.delta_time(), solver);
        });

    auto collect_spring_gradient = ecs.system<Spring, Solver>("Collect Spring Gradient")
        .with<Spring>()
        .kind(0)
        .each([](flecs::iter& it, size_t, Spring& spring, Solver& solver) {
            systems::collect_spring_gradient(spring, it.delta_time(), solver);
        });

    auto collect_mass = ecs.system<const Mass, const ParticleIndex, Solver>("Collect Mass")
        .with<Particle>()
        .kind(0)
        .each(systems::collect_mass);

    auto collect_spring_hessian = ecs.system<Spring, Solver>("Collect Spring Hessian")
        .with<Spring>()
        .kind(0)
        .each([](flecs::iter& it, size_t, Spring& spring, Solver& solver) {
            systems::collect_spring_hessian(spring, it.delta_time(), solver);
        });
        
    auto solve = ecs.system<Solver>("Solve Linear System")
        .kind(0)
        .run([](flecs::iter& it) {
            auto& solver = it.world().get_mut<Solver>();
            systems::solve(solver);
        });

    auto update_velocity = ecs.system<Velocity, const ParticleIndex, const Solver>("Update Velocity")
        .with<Particle>()
        .without<IsPinned>()
        .kind(0)
        .each([](flecs::iter& it, size_t, Velocity& v, const ParticleIndex& idx, const Solver& solver) {
            systems::update_velocity(v, idx, solver);
        });

    auto update_position = ecs.system<Position, const Velocity>("Update Position")
        .with<Particle>()
        .without<IsPinned>()
        .kind(0)
        .each([](flecs::iter& it, size_t, Position& x, const Velocity& v) {
            systems::update_position(x, v, it.delta_time());
        });

    // =========================================================================
    // Rendering systems (on graphics phases)
    // =========================================================================

    // cpu drawing
    ecs.system<Spring>("Draw Springs CPU")
        .kind(graphics::phase_on_render)
        .each([](Spring& s) {
            systems::draw_spring(s);
        }).disable();

    // upload positions each frame before rendering
    ecs.system("Upload Spring Positions")
        .kind(graphics::phase_pre_render)
        .run([&](flecs::iter& it) {
            // auto& gpu = it.world().get_mut<SpringRenderer>();
            systems::upload_spring_positions_to_gpu(ecs, gpu);
        });

    ecs.system("Draw Springs GPU")
        .kind(graphics::phase_on_render)
        .run([&](flecs::iter& it) {
            // auto& gpu = it.world().get_mut<SpringRenderer>();
            systems::draw_springs_gpu(gpu);
        });

    ecs.system<const Position, const Mass>("DrawParticles")
        .with<Particle>()
        .kind(graphics::phase_on_render)
        .each([](const Position& x, const Mass& m) {
            systems::draw_particle(x, m);
        }).disable();

    ecs.system("DrawTimingInfo")
        .kind(graphics::phase_post_render)
        .run(systems::draw_timing_info);

    // =========================================================================
    // LOOK HERE!!! - THE ALGORITHM in one place
    // =========================================================================

    // fixed 60 Hz simulation tick (decoupled from display rate)
    auto sim_tick = ecs.timer().interval(1.0f / 100.0f);

    // build particle grid cloth
    ecs.system("Create Cloth")
        .kind(flecs::OnStart)
        .run([&](flecs::iter& it) {
            // build cloth
            ClothConfig cfg;
            cfg.width = 50;
            cfg.height = 50;
            cfg.mass = 1.0;
            cfg.k_s = 1000.0;
            cfg.k_b = 100.0;
            // cfg.k_d = 0.1;
            cfg.spacing = 1;
            cfg.offset = Vector3r(-cfg.width/2, cfg.height/2, 0);
            create_cloth(ecs, cfg);

            auto& scene = it.world().get<Scene>();
            auto& solver = it.world().get_mut<Solver>();

            auto n = scene.num_particles;
            solver.particle_count = n;
            solver.b.resize(n * 3);
            solver.x.resize(n * 3);
            solver.A.resize(n * 3, n * 3);
        });

    // Initialize GPU renderer on first frame (after entities are committed)
    ecs.system("Init Spring Renderer")
        .kind(flecs::PreUpdate)
        .run([&](flecs::iter& it) {
            static bool initialized = false;
            if (!initialized) {
                systems::init_spring_gpu_renderer(ecs, gpu);
                initialized = true;
            }
        });

    ecs.system("Implicit Euler")
        .kind(flecs::PreUpdate)
        .tick_source(sim_tick)
        .run([&](flecs::iter& it) {
            const auto& scene = it.world().get<Scene>();
            auto dt = scene.timestep;

            // clear solver state for this frame
            auto& solver = it.world().get_mut<Solver>();
            solver.b.setZero();
            solver.triplets.clear();

            // build rhs, b
            // RHS = M*v_n + h*f_gravity - h*dE_dx
            collect_external_force.run(dt); // M*v_n + h*m*g
            collect_spring_gradient.run(dt); // -h*dE_dx (elastic forces)

            // build lhs, A
            // LHS = M + h^2 * ddE_ddx  (mass matrix + timestep^2 * Hessian)
            collect_mass.run(dt);
            // collect_spring_hessian.run(dt);
            
            // solve for x
            solve.run(); 

            // update states
            update_velocity.run(dt);
            update_position.run(dt);
        });

    // =========================================================================
    // Main loop
    // =========================================================================

    graphics::run_main_loop([]{});

    printf("Simulation ended.\n");
    return 0;
}
