// Mass-Spring Simulator Base

#include "flecs.h"
#include "graphics.h"

#include "components.h"
#include "systems.h"
#include "cloth.h"
#include <cstdio>
#include <limits>

// =========================================================================
// Main
// =========================================================================

int main() {
    printf("Hi from %s\n", __FILE__);

    flecs::world ecs;

    register_sim_components(ecs);

    // register cloth component with hooks (must be before any cloth creation)
    register_cloth_component(ecs);

    // scene setup with on_set hook for query initialization
    Eigen::Vector3r gravity(0, -9.81, 0);
    ecs.component<Scene>()
        .on_set([](flecs::entity e, Scene& scene) {
            auto world = e.world();
            scene.particle_query = world.query_builder<ParticleIndex>()
                .with<Particle>() // <Particle> is an empty tag, unlike <Spring>
                .cached()
                .build();
            scene.spring_query = world.query_builder<Spring>().cached().build();
        })
        .add(flecs::Singleton);
    ecs.set<Scene>({
        0.016,       // dt (timestep)
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
            if (gpu.instance_vbo) rlUnloadVertexBuffer(gpu.instance_vbo);
            if (gpu.vao) rlUnloadVertexArray(gpu.vao);
        })
        .add(flecs::Singleton);

    // initialize graphics module
    // (must call `::init()` before any raylib or gl specifics, otherwise crash)
    graphics::init(ecs);
    graphics::init_window(800, 600, "Base Simulator");
    graphics::Camera cam{};
    cam.target = Eigen::Vector3f(0.0f, 0.5f, 0.0f);
    graphics::Position cam_pos{};
    cam_pos.value = Eigen::Vector3f(50.0f, 10.0f, 50.0f);
    // TODO: we could make a slot for (optional) target entity so that
    // camera picks up `Position` component of the target (if exists)
    graphics::create_camera(ecs, "MainCamera", cam_pos, cam, true);

    // now set SpringRenderer to trigger on_set hook (after graphics init)
    ecs.set<SpringRenderer>({});
    auto& gpu = ecs.get_mut<SpringRenderer>();

    // ParticleRenderer with on_set hook for mesh, query, and shader initialization
    ecs.component<ParticleRenderer>()
        .on_set([](flecs::entity e, ParticleRenderer& gpu) {
            auto world = e.world();
            // cached query for per-frame position lookup
            gpu.position_query = world.query_builder<const Position, const ParticleIndex, const ParticleState>()
                .with<Particle>()
                .cached()
                .build();

            // generate icosphere mesh
            std::vector<float> vertices;
            std::vector<unsigned int> indices;
            systems::generate_icosphere(vertices, indices, 2); // last param for subdiv levle
            gpu.num_vertices = vertices.size() / 6;  // 6 floats per vertex (pos+normal)
            gpu.num_indices = indices.size();

            // create vao and upload mesh (static)
            gpu.vao = rlLoadVertexArray();
            rlEnableVertexArray(gpu.vao);

            gpu.mesh_vbo = rlLoadVertexBuffer(vertices.data(), vertices.size() * sizeof(float), false);

            // create element buffer (index buffer)
            glGenBuffers(1, &gpu.mesh_ebo);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gpu.mesh_ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

            // configure mesh attributes (location 0-1)
            constexpr int stride = 6 * sizeof(float);
            rlEnableVertexAttribute(0);
            rlSetVertexAttribute(0, 3, RL_FLOAT, false, stride, 0);  // position
            rlEnableVertexAttribute(1);
            rlSetVertexAttribute(1, 3, RL_FLOAT, false, stride, 3 * sizeof(float));  // normal

            rlDisableVertexArray();

            // load shader
            Shader shader = LoadShader(
                graphics::npath("resources/shaders/glsl300es/particle.vs").c_str(),
                graphics::npath("resources/shaders/glsl300es/particle.fs").c_str()
            );
            if (IsShaderValid(shader)) {
                gpu.shader_id = shader.id;
                gpu.u_viewproj_loc = GetShaderLocation(shader, "u_viewproj");
                gpu.u_color_loc = GetShaderLocation(shader, "u_color");
            }
        })
        .on_remove([](flecs::entity e, ParticleRenderer& gpu) {
            if (gpu.shader_id) UnloadShader({gpu.shader_id});
            if (gpu.instance_vbo) rlUnloadVertexBuffer(gpu.instance_vbo);
            if (gpu.mesh_ebo) glDeleteBuffers(1, &gpu.mesh_ebo);
            if (gpu.mesh_vbo) rlUnloadVertexBuffer(gpu.mesh_vbo);
            if (gpu.vao) rlUnloadVertexArray(gpu.vao);
        })
        .add(flecs::Singleton);

    // set ParticleRenderer to trigger on_set hook
    ecs.set<ParticleRenderer>({});

    // holds hovered/selected entity refs
    ecs.component<InteractionState>()
        .add(flecs::Singleton);
    ecs.set<InteractionState>({});

    auto particle_pick_query = ecs.query_builder<const Position, ParticleState>()
        .with<Particle>()
        .cached()
        .build();

    // =========================================================================
    // Simulation systems (we set .kind(0) to manually run them
    // =========================================================================

    auto collect_momentum = ecs.system<const Mass, const Velocity, const ParticleIndex, Solver>("Collect Momentum")
        .with<Particle>()
        .without<IsPinned>()
        .kind(0)
        .each([&](flecs::iter& it, size_t, const Mass& m, const Velocity& v, const ParticleIndex& idx, Solver& solver) {
            systems::collect_momentum(m, v, idx, solver);
        });

    auto collect_external_force = ecs.system<const Mass, const ParticleIndex, Solver>("Collect External Force")
        .with<Particle>()
        .without<IsPinned>()
        .kind(0)
        .each([&](flecs::iter& it, size_t, const Mass& m, const ParticleIndex& idx, Solver& solver) {
            systems::collect_external_force(m, idx, gravity, it.delta_time(), solver);
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
    ecs.system<Spring>("graphics::Draw Springs CPU")
        .kind(graphics::phase_on_render)
        .each([](Spring& s) {
            systems::draw_spring(s);
        }).disable();

    ecs.system<const Position, const Mass>("graphics::Draw Particles CPU")
        .with<Particle>()
        .kind(graphics::phase_on_render)
        .each([](const Position& x, const Mass& m) {
            systems::draw_particle(x, m);
        }).disable();

    // upload positions each frame before rendering
    ecs.system("graphics::Upload Spring Positions")
        .kind(graphics::phase_pre_render)
        .run([](flecs::iter& it) {
            auto& ctx = it.world().get_mut<SpringRenderer>();
            systems::upload_spring_positions_to_gpu(it.world(), ctx);
        }).disable(0);

    ecs.system("graphics::Draw Springs GPU")
        .kind(graphics::phase_on_render)
        .run([](flecs::iter& it) {
            auto& ctx = it.world().get_mut<SpringRenderer>();
            systems::draw_springs_gpu(ctx);
        }).disable(0);

    // upload particle positions each frame before rendering
    ecs.system("graphics::Upload Particle Positions")
        .kind(graphics::phase_pre_render)
        .run([](flecs::iter& it) {
            auto& ctx = it.world().get_mut<ParticleRenderer>();
            systems::upload_particle_positions_to_gpu(it.world(), ctx);
        }).disable();

    ecs.system("graphics::Draw Particles GPU")
        .kind(graphics::phase_on_render)
        .run([](flecs::iter& it) {
            auto& ctx = it.world().get_mut<ParticleRenderer>();
            systems::draw_particles_gpu(ctx);
        }).disable();

    ecs.system("graphics::Draw Timing Info")
        .kind(graphics::phase_post_render)
        .run(systems::draw_timing_info);

    // =========================================================================
    // Interaction systems
    // =========================================================================

    ecs.system("interaction::Pick Particles")
        .kind(flecs::OnLoad)
        // .before("graphics::update_camera")
        .run([&](flecs::iter& it) {
            auto& interaction = it.world().get_mut<InteractionState>();
            auto& renderer = it.world().get<ParticleRenderer>();

            // ray pick -> nearest particle
            Ray ray = GetMouseRay(GetMousePosition(), graphics::get_raylib_camera_const());
            float pick_radius = renderer.base_radius * interaction.pick_radius_scale;

            float closest = std::numeric_limits<float>::max();
            flecs::entity hovered = flecs::entity::null();

            particle_pick_query.each([&](flecs::entity e, const Position& pos, ParticleState&) {
                RayCollision hit = GetRayCollisionSphere(ray, systems::toRay3(pos.value), pick_radius);
                if (hit.hit && hit.distance < closest) {
                    closest = hit.distance;
                    hovered = e;
                }
            });

            // TODO: do gpu picking maybe?
            // i want no linear scan every frame looping all the particles..

            interaction.hovered = hovered;

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                if (hovered.is_alive()) {
                    interaction.selected = (interaction.selected == hovered)
                        ? flecs::entity::null()
                        : hovered;
                } else {
                    interaction.selected = flecs::entity::null();
                }
            }

            // if hit, lock camera drag interaction
            bool capture = IsMouseButtonDown(MOUSE_LEFT_BUTTON) && hovered.is_alive();
            it.world().query_builder<graphics::Camera>()
                .with<graphics::ActiveCamera>()
                .each([&](graphics::Camera& cam) {
                    cam.controls_enabled = !capture;
                });

            // update hover/selection flag bits
            constexpr uint8_t kInteractionMask = static_cast<uint8_t>(
                ParticleState::Hovered | ParticleState::Selected);

            particle_pick_query.each([&](flecs::entity e, const Position&, ParticleState& state) {
                uint8_t flags = static_cast<uint8_t>(state.flags & ~kInteractionMask);
                if (e == hovered) flags |= ParticleState::Hovered;
                if (interaction.selected == e) flags |= ParticleState::Selected;
                state.flags = flags;
            });
        }).disable();

    // =========================================================================
    // LOOK HERE!!! - THE ALGORITHM in one place
    // =========================================================================

    // fixed 60 Hz simulation tick (decoupled from display rate)
    // auto sim_tick = ecs.timer().interval(10.0f / 1000.0f);
    auto sim_tick = ecs.get<Scene>().dt;

    // build particle grid cloth
    // flecs::observer handles solver resizing automatically
    ecs.system("Create Cloth")
        .kind(flecs::OnStart)
        .immediate()
        .run([&](flecs::iter& it) {
            auto world = it.world();

            // create cloth using GridCloth component
            GridCloth cloth;
            cloth.width = 50;
            cloth.height = 50;
            cloth.spacing = 1.0f;
            cloth.mass = 1.0f;
            cloth.k_structural = 10000.0f;
            cloth.k_shear = 10000.0f;
            cloth.k_bending = 10.0f;
            cloth.k_damping = 0.05f;
            cloth.offset[0] = -cloth.width / 2.0f;
            cloth.offset[1] = cloth.height / 2.0f;
            cloth.offset[2] = 0.0f;
            cloth.pin_mode = 0;  // corners

            world.entity("Cloth").set<GridCloth>(cloth);
        });

    // collect system DOF - ensures solver is correctly sized
    // runs before simulation, reassigns particle indices if topology changed
    ecs.system("Collect System DOF")
        .kind(flecs::PreUpdate)
        .run([](flecs::iter& it) {
            auto world = it.world();
            auto& scene = world.get<Scene>();
            auto& solver = world.get_mut<Solver>();

            int n = scene.num_particles();
            int dof = n * 3;

            // only resize if needed
            if (solver.b.size() != dof) {
                // assign sequential indices to all particles
                int new_idx = 0;

                scene.particle_query.each([&](ParticleIndex& idx) {
                    idx.value = new_idx++;
                });

                // resize solver
                solver.b.resize(dof);
                solver.b.setZero();
                solver.x.setZero(dof);
                solver.x_prev.setZero(dof);
                solver.A.resize(dof, dof);

                printf("[Solver] Resized: %d particles, %d DOF\n", n, dof);
            }

            // TODO: should reset spring/particle renderer bc ordering might not be preserved

            solver.b.setZero();
            solver.triplets.clear();
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

            // build rhs, b
            // RHS = M * v_n + h * f_gravity - h * dE_dx
            collect_momentum.run(); // M * v_n
            collect_external_force.run(dt); // h * m * g
            collect_spring_gradient.run(dt); // h * -dE_dx (elastic forces)

            // build lhs, A
            // LHS = M + h^2 * ddE_ddx  (mass matrix + h^2 * Hessian)
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

    ecs.app()
        .enable_rest()
        .enable_stats()
        .run();

    printf("Simulation has ended?\n");
    return 0;
}
