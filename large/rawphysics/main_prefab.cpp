#include <flecs.h>

#include "graphics.h"

#include "kernels.hpp"
#include "types.h"

float randf(int n) {
    return static_cast<float>(rand() % n);
}

Real global_time = 0.0;
Real delta_time = 0.016;
int num_iter = 10;
int num_substeps = 100;

Vector3r gravity(0, -9.81f, 0);

using namespace phys::pbd;

namespace prefabs {
    struct Particle {};
    struct RigidBody {};
}

// should i use
//
// struct Position {
//     Vector3r value;
//     Vector3r prev; 
//     Vector3r init; 
// };
//
// instead of OldPosition?

void register_prefabs(flecs::world& ecs) {
    ecs.prefab<prefabs::Particle>()
        .add<Particle>() // empty tag
        .add<Position>()
        .add<OldPosition>()
        .add<LinearVelocity>()
        // .add<Acceleration>()
        .add<Mass>()
        .add<InverseMass>();
}

flecs::entity add_distance_constraint(
    flecs::world& ecs, flecs::entity& e1, flecs::entity& e2, Real stiffness = 1.0f) {

    // Flecs 4.1.1+: get<T>() returns reference, not pointer
    auto rest_distance = (e1.get<Position>().value - e2.get<Position>().value).norm();

    auto e = ecs.entity();
    e.add<Constraint>();

    e.add<DistanceConstraint>();
    // Flecs 4.1.1+: get_mut<T>() returns reference, not pointer
    auto& c = e.get_mut<DistanceConstraint>();
    c.e1 = e1;
    c.e2 = e2;
    c.rest_distance = rest_distance;
    c.stiffness = stiffness;

    return e;
}

flecs::entity add_particle(flecs::world& ecs, const Vector3r& pos = Vector3r::Zero(), Real mass = 1.0f) {
    return ecs.entity()
                .is_a<prefabs::Particle>()
                .set<Position>({pos})
                .set<OldPosition>({pos})
                .set<LinearVelocity>({Vector3r::Zero()})
                // .set<Acceleration>({Vector3r::Zero()})
                .set<Mass>({mass})
                .set<InverseMass>({1 / mass});
}

inline Vector3 e2r(const Vector3r& v) {
    return {(float)v.x(), (float)v.y(), (float)v.z()};
}

inline Vector3r e2r(const Vector3& v) {
    return {(Real)v.x, (Real)v.y, (Real)v.z};
}

int main() {
    flecs::world ecs;

    // ecs.app().enable_rest().enable_stats();
    ecs.import<flecs::stats>();
    ecs.set<flecs::Rest>({});

    
    // ecs.import<flecs::metrics>();

    graphics::init(ecs);
    graphics::init_window(800, 600, "muller2007position");


    register_prefabs(ecs);
    ecs.set<Scene>({delta_time, 1, 1, gravity});

    auto p1 = add_particle(ecs, Vector3r(0, 4, 0), 1.0f);
    auto p2 = add_particle(ecs, Vector3r(-1, 4, 0), 1.0f * std::pow(10, GetRandomValue(0, 3)));
    auto p3 = add_particle(ecs, Vector3r(-2, 4, 0), 1.0f * std::pow(10, GetRandomValue(0, 3)));
    auto p4 = add_particle(ecs, Vector3r(-3, 4, 0), 1.0f * std::pow(10, GetRandomValue(0, 3)));

    p1.add<IsPinned>();
    p1.set<InverseMass>({0});

    add_distance_constraint(ecs, p1, p2, 1e+5 * std::pow(10, GetRandomValue(0, 2)));
    add_distance_constraint(ecs, p2, p3, 1e+5 * std::pow(10, GetRandomValue(0, 2)));
    add_distance_constraint(ecs, p3, p4, 1e+5 * std::pow(10, GetRandomValue(0, 2)));
    // add_distance_constraint(ecs, p1, p4, 1e+7);
    // add_distance_constraint(ecs, p2, p4, 1e+7);

    ////////// system registration
    ecs.system("progress").kind(flecs::OnUpdate).run([](flecs::iter& it) {
        auto params = it.world().get_mut<Scene>();
        params->elapsed += params->timestep;
    });

    auto clear_acc =
        ecs.system<Acceleration>("clear acceleration")
            .with<Particle>()
            .kind(0)
            .each([&](Acceleration& a) { particle_clear_acceleration(a.value, gravity); });

    auto integrate =
        ecs.system<Position, OldPosition, Velocity, const Mass, const Acceleration>("integrate")
            .with<Particle>()
            .without<IsPinned>()
            .kind(0)
            .each([&](Position& x,
                      OldPosition& x_old,
                      Velocity& v,
                      const Mass mass,
                      const Acceleration& acc) {
                particle_integrate(x.value,
                                   x_old.value,
                                   v.value,
                                   mass.value,
                                   acc.value,
                                   delta_time / num_substeps);
            });

    auto clear_lambda =
        ecs.system<DistanceConstraint>("clear constraint lambda")
            .kind(0)
            .each([&](DistanceConstraint& c) { particle_clear_constraint_lambda(c); });

    auto solve_constraint =
        ecs.system<DistanceConstraint>("constraint solve").kind(0).each([&](DistanceConstraint& c) {
            // particle_solve_constraint(c, delta_time / num_substeps);
            auto e1 = c.e1;
            auto e2 = c.e2;
            
            auto x1 = e1.get_mut<Position>();
            auto x2 = e2.get_mut<Position>();

            auto w1 = e1.get<InverseMass>();
            auto w2 = e2.get<InverseMass>();

            particle_solve_distance_constraint(x1->value, x2->value, c.lambda, w1->value, w2->value, c.stiffness, c.rest_distance, delta_time / num_substeps);
        });

        // ecs.system<Position, Position>().each([](Position& p1, Position& p2) {
        //     auto sum = p1.value + p2.value;
        // });

    auto handle_ground_collision =
        ecs.system<Position, const OldPosition>("ground collision")
            .with<Particle>()
            .kind(0)
            .each([&](Position& x, const OldPosition x_old) {
                particle_ground_collision(x.value, x_old.value, delta_time / num_substeps);
            });

    auto update_velocity =
        ecs.system<Velocity, const Position, const OldPosition>("update velocity")
            .with<Particle>()
            .kind(0)
            .each([&](Velocity& v, const Position& x, const OldPosition& x_old) {
                particle_update_velocity(v.value, x.value, x_old.value, delta_time / num_substeps);
            });

    ecs.system("simulation loop").run([&](flecs::iter&) {
        auto sub_dt = delta_time / num_substeps;

        for (int i = 0; i < num_substeps; ++i) {
            clear_acc.run();
            integrate.run();

            clear_lambda.run();

            for (int j = 0; j < num_iter; ++j) {
                solve_constraint.run();
            }

            handle_ground_collision.run();
            update_velocity.run();
        }
    });

    //////// drawings below

    // Flecs 4.1.1+: get_mut<T>() returns reference, not pointer
    ecs.system<DistanceConstraint>("draw_distance_constraint")
        .kind(graphics::phase_on_render)
        .each([](DistanceConstraint& c) {
            auto e1 = c.e1;
            auto e2 = c.e2;

            auto x1 = e1.get_mut<Position>().value;
            auto x2 = e2.get_mut<Position>().value;

            float base_thickness = 0.0005f;
            float thickness = base_thickness * std::pow(c.stiffness, 1.0f / 3.0f);

            // normalize lambda for color interpolation using log scale
            float lambda_abs = std::abs(c.lambda);
            float t = lambda_abs > 1e-10f ? std::min(1.0f, -std::log10(lambda_abs) / 10.0f)
                                          : 1.0f; // fully green when lambda is very close to 0

            // use built-in color interpolation
            Color color = ColorLerp(RED, GREEN, t);

            // DrawLine3D({x1.x(), x1.y(), x1.z()}, {x2.x(), x2.y(), x2.z()}, GREEN);
            DrawCylinderEx(e2r(x1), e2r(x2), thickness, thickness, 5, color);
            // DrawCylinderWiresEx(e2r(x1), e2r(x2), thickness, thickness, 10, color);
        });

    ecs.system<const Position, const Mass>("draw_particle")
        .with<Particle>()
        .kind(graphics::phase_on_render)
        .each([](const Position& x, const Mass m) {
            DrawSphere(e2r(x.value), 0.05 * std::pow(m.value, 1.0f / 3.0f), BLUE);
            // DrawSphereWires(e2r(x.value), 0.05 * std::pow(m.value, 1.0f / 3.0f), 16, 16, BLUE);
        });

    ecs.system<const Position, const Mass>("draw mass info")
        .with<Particle>()
        .kind(graphics::phase_post_render)
        .each([](const Position& x, const Mass& m) {
            Vector3 textPos = e2r(x.value);
            textPos.y += 0.2f;
            Vector2 screenPos = GetWorldToScreen(textPos, graphics::detail::camera);

            char id_buffer[32];

            const char* label = (sprintf(id_buffer, "%0.2f", m.value), id_buffer);

            DrawText(label, screenPos.x, screenPos.y, 20, BLUE);
        });

    // Flecs 4.1.1+: get_mut<T>() returns reference, not pointer
    ecs.system<DistanceConstraint>("draw_constraint_lambda")
        .kind(graphics::phase_post_render)
        .each([](DistanceConstraint& c) {
            auto e1 = c.e1;
            auto e2 = c.e2;

            auto x1 = e1.get_mut<Position>().value;
            auto x2 = e2.get_mut<Position>().value;

            auto x = (x1 + x2) / 2;

            Vector3 textPos = e2r(x);
            Vector2 screenPos = GetWorldToScreen(textPos, graphics::detail::camera);

            char id_buffer[32];

            const char* label = (sprintf(id_buffer, "%0.6f", c.lambda), id_buffer);

            DrawText(label, screenPos.x, screenPos.y, 20, BLUE);
        });

    ecs.system("you can do it").kind(graphics::phase_post_render).run([](flecs::iter& it) {
        auto params = it.world().get<Scene>();

        char buffer[64];
        snprintf(buffer, sizeof(buffer), "Elapsed time: %.2f", params->elapsed);
        DrawText(buffer, 20, 40, 20, DARKGREEN);

        DrawText("dt=1/60 num_iter=10 num_substeps=100\nuses random config for each run\nlambda visualized by red-green color "
                 "map\nInput: W,A,S,D,Q,E,Up,Down,Left,Top,MouseDrag",
                 20,
                 100,
                 20,
                 DARKGREEN);
    });

    // ecs.import <flecs::stats>(); % this causes problems on web build
    // ecs.set<flecs::Rest>({});

    graphics::run_main_loop([]() { global_time += delta_time; });

    printf("Simulation ended.\n");
    return 0;
}

// #if defined(__EMSCRIPTEN__)
// #include <emscripten/emscripten.h>
// extern "C" {
//     EMSCRIPTEN_KEEPALIVE void emscripten_shutdown() {
//         graphics::close_window();
//     }
// }
// #endif