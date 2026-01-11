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

    // Scene setup with on_set hook for query initialization
    Vector3r gravity(0, -9.81, 0);
    ecs.component<Scene>()
        .on_set([](flecs::entity e, Scene& scene) {
            auto world = e.world();
            scene.particle_query = world.query_builder<Particle>().cached().build();
            scene.spring_query = world.query_builder<Spring>().cached().build();
        })
        .add(flecs::Singleton);
    ecs.set<Scene>({
        0.01666,       // dt (timestep)
        gravity        // gravity
    });

    ecs.component<Solver>()
        .add(flecs::Singleton);
    auto& solver = ecs.ensure<Solver>();
    solver.b.setZero();
    solver.A.setZero();
    solver.cg_tolerance = 1e-6;
    solver.cg_max_iter = 100;

    // SpringRenderer with on_set hook for query and shader initialization
    ecs.component<SpringRenderer>()
        .on_set([](flecs::entity e, SpringRenderer& gpu) {
            auto world = e.world();
            // Cached query for per-frame position lookup
            gpu.position_query = world.query_builder<const Position, const ParticleIndex>()
                .cached()
                .build();
            // Load shader (requires graphics to be initialized first)
            Shader shader = LoadShader(
                graphics::npath("resources/shaders/glsl300es/spring.vs").c_str(),
                graphics::npath("resources/shaders/glsl300es/spring.fs").c_str()
            );
            if (IsShaderValid(shader)) {
                gpu.shader_id = shader.id;
                gpu.u_viewproj_loc = GetShaderLocation(shader, "u_viewproj");
                gpu.u_strain_scale_loc = GetShaderLocation(shader, "u_strain_scale");
            }
        })
        .on_remove([](flecs::entity e, SpringRenderer& gpu) {
            if (gpu.shader_id) UnloadShader({gpu.shader_id});
            if (gpu.vbo) rlUnloadVertexBuffer(gpu.vbo);
            if (gpu.vao) rlUnloadVertexArray(gpu.vao);
        })
        .add(flecs::Singleton);

    // Initialize graphics (must be before SpringRenderer set for shader loading)
    graphics::init(ecs);
    graphics::init_window(800, 600, "Implicit Euler");
    graphics::create_camera(ecs, "MainCamera", {
        {50.0f, 10.0f, 50.0f},  // position
        {0.0f, 0.5f, 0.0f},     // target
    }, true);

    // Now set SpringRenderer to trigger on_set hook (after graphics init)
    ecs.set<SpringRenderer>({});
    auto& gpu = ecs.get_mut<SpringRenderer>();

    // =========================================================================
    // Simulation systems (we set .kind(0) to manually run them
    // =========================================================================

    auto collect_external_force = ecs.system<const Mass, const Velocity, const ParticleIndex, Solver>("Collect External Force")
        .with<Particle>()
        .without<IsPinned>()
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
        .without<IsPinned>()
        .kind(0)
        .each(systems::collect_mass);

    auto collect_spring_hessian = ecs.system<Spring, Solver>("Collect Spring Hessian")
        .with<Spring>()
        .kind(0)
        .each([](flecs::iter& it, size_t, Spring& spring, Solver& solver) {
            systems::collect_spring_hessian(spring, it.delta_time(), solver);
        });
        
    auto solve = ecs.system("Solve Linear System")
        .kind(0)
        .run([](flecs::iter& it) {
            // `Solver` here is a singleton
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

    ecs.system<const Position, const Mass>("DrawParticles")
        .with<Particle>()
        .kind(graphics::phase_on_render)
        .each([](const Position& x, const Mass& m) {
            systems::draw_particle(x, m);
        }).disable();

    // upload positions each frame before rendering
    ecs.system("Upload Spring Positions")
        .kind(graphics::phase_pre_render)
        .run([&](flecs::iter& it) {
            // auto& gpu = it.world().get_mut<SpringRenderer>();
            systems::upload_spring_positions_to_gpu(ecs, gpu);
        }).disable(0);

    ecs.system("Draw Springs GPU")
        .kind(graphics::phase_on_render)
        .run([&](flecs::iter& it) {
            // auto& gpu = it.world().get_mut<SpringRenderer>();
            systems::draw_springs_gpu(gpu);
        }).disable(0);

    ecs.system("DrawTimingInfo")
        .kind(graphics::phase_post_render)
        .run(systems::draw_timing_info);

    // =========================================================================
    // LOOK HERE!!! - THE ALGORITHM in one place
    // =========================================================================

    // fixed 60 Hz simulation tick (decoupled from display rate)
    // auto sim_tick = ecs.timer().interval(10.0f / 1000.0f);
    auto sim_tick = ecs.get<Scene>().dt;

    // Build particle grid cloth (immediate mode to ensure query counts are correct)
    ecs.system("Create Cloth")
        .kind(flecs::OnStart)
        .immediate()
        .run([&](flecs::iter& it) {
            auto world = it.world();

            // Suspend deferred operations so entities are created immediately
            world.defer_suspend();

            ClothConfig cfg;
            cfg.width = 10;
            cfg.height = 40;
            cfg.mass = 1;
            cfg.k_s = 10000.0;
            cfg.k_b = 10.0;
            cfg.k_d = 0.05;
            cfg.spacing = 1;
            cfg.offset = Vector3r(-cfg.width/2.0, cfg.height/2.0, 0);
            create_cloth(ecs, cfg);

            world.defer_resume();

            // Now query counts are correct
            auto& scene = world.get<Scene>();
            auto& solver = world.get_mut<Solver>();

            int n = scene.num_particles();
            solver.b.resize(n * 3);
            solver.x.setZero(n * 3);
            solver.x_prev.setZero(n * 3);
            solver.A.resize(n * 3, n * 3);
        });

    ecs.system("Implicit Euler")
        .kind(flecs::PreUpdate)
        .tick_source(sim_tick)
        .run([&](flecs::iter& it) {
            auto& scene = it.world().get_mut<Scene>();
            auto dt = scene.dt;

            // accumulate simulation time and frame count
            scene.sim_time += dt;
            scene.frame_count++;

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
            collect_spring_hessian.run(dt);
            
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
