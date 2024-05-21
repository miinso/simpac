#include "flecs.h"

#include <iostream>

#include "deps/flecs_components_input.h"
#include "deps/flecs_components_graphics.h"
#include "deps/flecs_components_gui.h"
#include "deps/flecs_components_transform.h"
#include "deps/flecs_components_geometry.h"
#include "deps/flecs_components_physics.h"
#include "deps/flecs_systems_physics.h"
#include "deps/flecs_game.h"
#include "deps/flecs_systems_sokol.h"

// #include <Eigen/Dense>

using namespace flecs::components;

static const float ActorSize = 0.7;

// Component types
using Position = transform::Position3;
using Rotation = transform::Rotation3;
using Velocity = physics::Velocity3;

// struct Velocity {
//   double x;
//   double y;
// };

struct Gravity {
    double x;
    double y;
    double z;
};

vec3 asd;

// Tag types
struct Eats {};
struct Apples {};
struct Actor { };

// Scope for systems
struct systems { };

struct enemies { };

namespace prefabs {
    struct Tree {
        struct Trunk { };
        struct Canopy { };
    };

    struct Plane { };
    struct Tile { };
    struct Path { };
    struct Enemy { };

    struct materials {
        struct Beam { };
        struct Metal { };
        struct CannonHead { };
        struct LaserLight { };
    };

    struct Particle { };
    struct Bullet { };
    struct NozzleFlash { };
    struct Smoke { };
    struct Spark { };
    struct Ion { };
    struct Bolt { };

    struct Turret {
        struct Base { };
        struct Head { };
    };

    struct Cannon {
        struct Head {
            struct BarrelLeft { };
            struct BarrelRight { };
        };
        struct Barrel { };
    };

    struct Laser {
        struct Head {
            struct Beam { };
        };
    };
}

void init_ui(flecs::world& ecs) {
    graphics::Camera camera_data = {};
    camera_data.set_up(0, 1, 0);
    camera_data.set_fov(20);
    camera_data.near_ = 1.0;
    camera_data.far_ = 100.0;
    auto camera = ecs.entity("Camera")
        .add(flecs::game::CameraController)
        .set<Position>({0, 8.0, -9.0})
        .set<Rotation>({-0.5})
        .set<graphics::Camera>(camera_data);

    graphics::DirectionalLight light_data = {};
    light_data.set_direction(0.3, -1.0, 0.5);
    light_data.set_color(0.98, 0.95, 0.8);
    light_data.intensity = 1.0;
    auto light = ecs.entity("DirectionalLight")
        .set<graphics::DirectionalLight>(light_data);

    gui::Canvas canvas_data = {};
    canvas_data.width = 1400;
    canvas_data.height  = 1000;
    canvas_data.title = (char*)"simpac v0.0.1";
    canvas_data.camera = camera.id();
    canvas_data.directional_light = light.id();
    canvas_data.ambient_light = {0.06, 0.05, 0.18};
    canvas_data.background_color = {0.15, 0.4, 0.6};
    canvas_data.fog_density = 1.0;
    ecs.entity()
        .set<gui::Canvas>(canvas_data);
}

void init_prefabs(flecs::world& ecs) {
    ecs.prefab<prefabs::materials::Metal>()
        .set<graphics::Rgb>({0.1, 0.1, 0.1})
        .set<graphics::Specular>({1.5, 128});

    ecs.prefab<prefabs::Enemy>()
        .is_a<prefabs::materials::Metal>()
        .add<Actor>()
        .set_override<graphics::Rgb>({0.05, 0.05, 0.05})
        .set<geometry::Box>({ActorSize, ActorSize, ActorSize})
        .set<graphics::Specular>({4.0, 512});
        // .set<SpatialQuery, Bullet>({g->center, g->size})
        // .set_override<HitCooldown>({})
        // .add<SpatialQueryResult, Bullet>()
        // .override<SpatialQueryResult, Bullet>();

    ecs.prefab<prefabs::Plane>()
        .is_a<prefabs::materials::Metal>()
        .set<geometry::Box>({10, 0, 10})
        .set_override<graphics::Rgb>({0.9, 0.9, 0.9})
        // .set<Position>({0,0,0})
        .set<graphics::Specular>({4.0, 512});
}

// Init systems
void init_systems(flecs::world& ecs) {
    ecs.scope<systems>([&](){ // Keep root scope clean
        ecs.system<Velocity, const Gravity>("Gravity")
            .term<Actor>()
            .term_at(2).singleton()
            .each([](flecs::iter& it, size_t, Velocity &v, const Gravity &g){
                v.x += g.x * it.delta_time();
                v.y += g.y * it.delta_time();
                v.z += g.z * it.delta_time();
            });

        ecs.system<Position, const Velocity>("Integrate")
            .term<Actor>()
            .each([](flecs::iter& it, size_t, Position& p, const Velocity &v) {
                p.x += v.x * it.delta_time();
                p.y += v.y * it.delta_time();
                p.z += v.z * it.delta_time();
                
            });

        ecs.system<Position, Velocity>("Bottom")
            .term<Actor>()
            .each([](flecs::iter& it, size_t, Position& p, Velocity& v){
                if ((p.y) < 0) {
                    p.y = 5;
                    v.y = 0;
                }
            });

    });
}

int main(int argc, char *argv[]) {
  // Create the world
    flecs::world ecs(argc, argv);
    flecs::log::set_level(1);
    ecs.set<Gravity>({ 0, -9.81, 0 });

  // Register system
//   ecs.system<Position, Velocity>().each([](Position &p, Velocity &v) {
//     p.x += v.x;
//     p.y += v.y;
//   });

//   ecs.system<Position, Velocity>().iter(
//       [](flecs::iter &it, Position *p, Velocity *v) {
//         for (auto i : it) {
//           std::cout << "Moved " << it.entity(i).name() << " to {" << p[i].x
//                     << ", " << p[i].y << "}" << std::endl;
//         }
//       });

  // Create an entity with name Bob, add Position and food preference
//   flecs::entity Bob = ecs.entity("Bob")
//                           .set(Position{0, 0, 0})
//                           .set(Velocity{1, 2})
//                           .add<Eats, Apples>();

//   // Show us what you got
//   std::cout << Bob.name() << "'s got [" << Bob.type().str() << "]\n";

//   // Run systems twice. Usually this function is called once per frame
//   ecs.progress();
//   ecs.progress();

  // See if Bob has moved (he has)
//   const Position *p = Bob.get<Position>();
//   std::cout << Bob.name() << "'s position is {" << p->x << ", " << p->y
//             << "}\n";

  // Output
  //  Bob's got [Position, Velocity, (Identifier,Name), (Eats,Apples)]
  //  Bob's position is {2, 4}

//   ecs.system<Position, Velocity>("Integration")
//       .kind(flecs::OnUpdate)
//       .each([](Position &p, Velocity &v) {
//         std::cout << "Semi-implicit Euler Integration" << std::endl;
//       });
    
        // .set<Velocity>({0,0,0});

    ecs.import<flecs::components::transform>();
    ecs.import<flecs::components::graphics>();
    ecs.import<flecs::components::geometry>();
    ecs.import<flecs::components::gui>();
    ecs.import<flecs::components::physics>();
    ecs.import<flecs::components::input>();
    ecs.import<flecs::systems::transform>();
    ecs.import<flecs::systems::physics>();
    ecs.import<flecs::monitor>();
    ecs.import<flecs::game>();
    ecs.import<flecs::systems::sokol>();
    // ecs.import<flecs::components::physics>();
    init_ui(ecs);
    init_prefabs(ecs);
    init_systems(ecs);

    ecs.entity("rb").is_a<prefabs::Enemy>()
        .set<Position>({
            0, 1.2, 0
        })
        .set<Velocity>({0,0,0});

    ecs.entity("floor").is_a<prefabs::Plane>().set<Position>({0,0,0});

    ecs.app()
        .enable_rest()
        .target_fps(10)
        .run();

}