#include <flecs.h>

#include "graphics.h"

#include "entity.hpp"
#include "kernels.hpp"
#include "types.h"

#include <graphics/graphics.h>
#include <iostream>

float randf (int n) {
    return static_cast<float> (rand () % n);
}

Real global_time = 0.0;
Real delta_time  = 0.016;
int num_iter     = 2;
int num_substeps = 1;

Vector3r gravity (0, -9.81f, 0);

using namespace phys::pbd;
using namespace phys::pbd::rigidbody;


// should i use
//
// struct Position {
//     Vector3r value;
//     Vector3r prev;
//     Vector3r init;
// };
//
// instead of OldPosition?


inline Vector3 e2r (const Vector3r& v) {
    return { (float)v.x (), (float)v.y (), (float)v.z () };
}

inline Vector3r e2r (const Vector3& v) {
    return { (Real)v.x, (Real)v.y, (Real)v.z };
}

int main () {
    flecs::world ecs;

    // ecs.app().enable_rest().enable_stats();
    // ecs.import <flecs::stats> ();
    // ecs.set<flecs::Rest> ({});

    graphics::init (ecs);
    graphics::init_window (800, 600, "muller2007position");

    register_prefabs (ecs);
    ecs.set<Scene> ({ delta_time, 1, 1, gravity });

    // auto rb1                               = add_rigid_body (ecs);
    // rb1.get_mut<AngularVelocity> ()->value = Vector3r (0, 3, 0);

    auto rb1                        = add_rigid_body (ecs);
    rb1.get_mut<Position> ()->value = Vector3r (0.2, 0, 0);

    auto fixed1 = add_rigid_body (ecs, { 0, 3, 0 });
    // rb1.get_mut<LinearVelocity> ()->value  = Vector3r (0, 1, 0);
    fixed1.get_mut<AngularVelocity> ()->value = Vector3r (0, 1, 0.2);
    fixed1.add<IsPinned> ();

    Constraint cc1; // component
    // flecs::entity add_positional_constraint();
    rb_positional_constraint_init (
    cc1, rb1, fixed1, { 0.5, 0.5, 0.5 }, Vector3r::Zero (), 1e-2, 2);
    ecs.entity ().set<Constraint> (cc1); // entity now

    ////////// system registration
    ecs.system ("progress").kind (flecs::OnUpdate).run ([] (flecs::iter& it) {
        auto params = it.world ().get_mut<Scene> ();
        params->elapsed += params->timestep;
    });

    auto clear_force =
    ecs.system<Forces, LinearForce, AngularForce> ("clear force")
    .with<RigidBody> ()
    .kind (0) // `0` means it's gonna be called manually, see below
    .each ([] (Forces& forces, LinearForce& f, AngularForce& tau) {
        forces.a.clear ();
        rb_clear_force (f.value, tau.value);
    });

    auto add_gravity =
    ecs.system<Forces, Mass> ("add_gravity").with<RigidBody> ().kind (0).each ([] (Forces& forces, Mass& mass) {
        Physics_Force pf;
        Vector3r gravity (0, -10, 0);
        pf.force    = gravity * mass.value;
        pf.position = Vector3r::Zero ();
        forces.a.push_back (pf);
    });

    // compute forces. collect external forces or gravity
    auto compute_external_forces =
    ecs.system<Forces, LinearForce, AngularForce> ("compute external forces")
    .with<RigidBody> ()
    .kind (0) // `0` means it's gonna be called manually, see below
    .each ([] (flecs::entity e, Forces& forces, LinearForce& f, AngularForce& tau) {
        f.value   = rb_calculate_external_force (e);
        tau.value = rb_calculate_external_torque (e);
    });

    auto pbd_integrate =
    ecs
    .system<Position, Orientation, LinearVelocity, AngularVelocity, OldPosition, OldOrientation, const LocalInertia,
    const LocalInverseInertia, const LinearForce, const AngularForce, const Mass> ("integrate")
    .with<RigidBody> ()
    .without<IsPinned> ()
    .kind (0)
    .each ([&] (Position& x, Orientation& q, LinearVelocity& v,
           AngularVelocity& omega, OldPosition& x_old, OldOrientation& q_old,
           const LocalInertia& I_body, const LocalInverseInertia& I_body_inv,
           const LinearForce& f, const AngularForce& tau, const Mass mass) {
        x_old.value = x.value;
        q_old.value = q.value;
        // std::cout << x_old.value << std::endl;
        rb_integrate (x.value, q.value, v.value, omega.value, I_body.value,
        I_body_inv.value, f.value, tau.value, mass.value, delta_time / num_substeps);
    });

    auto clear_lambda =
    ecs.system<Constraint> ("clear constraint lambda").kind (0).each ([&] (Constraint& c) {
        rb_clear_constraint_lambda (c);
    });

    auto solve_constraint =
    ecs.system<Constraint> ("constraint solve").kind (0).each ([&] (Constraint& c) {
        rb_solve_constraint (c, delta_time / num_substeps);
    });

    // auto handle_ground_collision =
    //     ecs.system<Position, const OldPosition>("ground collision")
    //         .with<Particle>()
    //         .kind(0)
    //         .each([&](Position& x, const OldPosition x_old) {
    //             particle_ground_collision(x.value, x_old.value, delta_time / num_substeps);
    //         });

    auto update_velocity =
    ecs
    .system<LinearVelocity, OldLinearVelocity, AngularVelocity, OldAngularVelocity,
    const Position, const OldPosition, const Orientation, const OldOrientation> ("update velocity")
    .with<RigidBody> ()
    .kind (0)
    .each ([&] (LinearVelocity& v, OldLinearVelocity& v_old, AngularVelocity& omega,
           OldAngularVelocity& omega_old, const Position& x, const OldPosition& x_old,
           const Orientation& q, const OldOrientation& q_old) {
        // particle_update_velocity(v.value, x.value, x_old.value, delta_time / num_substeps);
        rb_update_velocity (v.value, v_old.value, omega.value, omega_old.value,
        x.value, x_old.value, q.value, q_old.value, delta_time / num_substeps);
    });

    ecs.system ("simulation loop").run ([&] (flecs::iter&) {
        auto sub_dt = delta_time / num_substeps;

        for (int i = 0; i < num_substeps; ++i) {
            clear_force.run ();
            add_gravity.run ();
            compute_external_forces.run ();
            pbd_integrate.run ();

            clear_lambda.run ();

            for (int j = 0; j < num_iter; ++j) {
                solve_constraint.run ();
            }

            // handle_ground_collision.run();
            update_velocity.run ();
        }
    });

    //////// drawings below

    auto mmm = LoadModelFromMesh (GenMeshCube (1, 1, 1));


    ecs.system<const Position, const Orientation> ("draw_physics_mesh")
    .with<RigidBody> ()
    .kind (graphics::OnRender)
    .each ([&] (const Position& x, const Orientation& q) {
        // draw physics mesh
        // DrawCube(e2r(x.value), q.value.w(), 1, 1, BLUE);
        // auto m = body.physics_mesh;
        // DrawMesh(x, q, m);

        auto aa = Eigen::AngleAxisd (q.value);

        DrawModelWiresEx (mmm, e2r (x.value), e2r (aa.axis ()),
        aa.angle () * RAD2DEG, e2r (Vector3r::Ones ()), BLUE);
    });

    auto default_mat = LoadMaterialDefault ();
    ecs
    .system<const Position, const Orientation, const Mesh0> (
    "draw_physics_mesh2")
    .with<RigidBody> ()
    .kind (graphics::OnRender)
    .each ([&] (flecs::entity e, const Position& x, const Orientation& q, const Mesh0& mesh0) {
        // draw physics mesh
        // DrawCube(e2r(x.value), q.value.w(), 1, 1, BLUE);
        // auto m = body.physics_mesh;
        // DrawMesh(x, q, m);

        auto aa = Eigen::AngleAxisd (q.value);
        auto translation = MatrixTranslate (x.value.x (), x.value.y (), x.value.z ());
        auto rotation = MatrixRotate (e2r (aa.axis ()), aa.angle () * RAD2DEG);
        auto tf       = MatrixMultiply (rotation, translation);
        // mesh0.m;
        DrawMesh (mesh0.m, default_mat, tf);
    });

    // ecs.system<const Position, const Orientation>("draw_render_mesh")
    //     .with<RigidBody>()
    //     .kind(graphics::OnRender)
    //     .each([&](const Position& x, const Orientation q) {
    //         // draw render mesh
    //         auto angle = 2.0 * acos(q.value.w());
    //         auto s = sqrt(1.0f - q.value.w() * q.value.w());
    //         auto axis = Vector3r(q.value.x() / s, q.value.y() / s, q.value.z() / s);

    //         DrawModelEx(mmm, e2r(x.value), e2r(axis), angle, {1,1,1}, BLUE);
    //         // auto m = body.render_mesh;
    //         // DrawMesh(x, q, m);
    //     });

    ecs.system<const Position, const Mass> ("draw mass info")
    .with<RigidBody> ()
    .kind (graphics::PostRender)
    .each ([] (const Position& x, const Mass& m) {
        Vector3 textPos = e2r (x.value);
        textPos.y += 0.2f;
        Vector2 screenPos = GetWorldToScreen (textPos, graphics::camera);

        char id_buffer[32];

        const char* label = (sprintf (id_buffer, "%0.2f", m.value), id_buffer);

        DrawText (label, screenPos.x, screenPos.y, 20, BLUE);
    });

    ecs.system<Constraint> ("draw constraint").kind (graphics::OnRender).each ([] (const Constraint& c) {
        auto e1 = c.e1;
        auto e2 = c.e2;
        auto x1 = e1.get<Position> ()->value;
        auto x2 = e2.get<Position> ()->value;
        auto q1 = e1.get<Orientation> ()->value;
        auto q2 = e2.get<Orientation> ()->value;

        auto r1_wc = x1 + q1._transformVector (c.positional_constraint.r1_lc);
        auto r2_wc = x2 + q2._transformVector (c.positional_constraint.r2_lc);

        float lambda_abs = std::abs (c.positional_constraint.lambda);
        float t          = lambda_abs > 1e-10f ?
                 std::min (1.0f, -std::log10 (lambda_abs) / 10.0f) :
                 1.0f; // fully green when lambda is very close to 0

        Color color = ColorLerp (RED, GREEN, t);
        DrawCylinderEx (e2r (r1_wc), e2r (r2_wc), 0.01, 0.01, 5, color);
    });

    ecs.system ("you can do it").kind (graphics::PostRender).run ([] (flecs::iter& it) {
        auto params = it.world ().get<Scene> ();

        char buffer[64];
        snprintf (buffer, sizeof (buffer), "Elapsed time: %.2f", params->elapsed);
        DrawText (buffer, 20, 40, 20, DARKGREEN);

        DrawText ("dt=1/60 num_iter=10 num_substeps=100\nuses random config "
                  "for each run\nlambda visualized by red-green color "
                  "map\nInput: W,A,S,D,Q,E,Up,Down,Left,Top,MouseDrag",
        20, 100, 20, DARKGREEN);
    });

    // key input

    ecs.system ("key handle").kind (graphics::PreRender).run ([&] (flecs::iter& it) {
        if (IsKeyDown (KEY_SPACE)) {
            // throw_object (ecs);
        }
        // it.fini();
    });

    example_util_install_floor (ecs);
    example_util_throw_object (ecs);

    // ecs.import <flecs::stats>(); % this causes problems on web build
    // ecs.set<flecs::Rest>({});

    graphics::run_main_loop ([] () { global_time += delta_time; });

    printf ("Simulation ended.\n");
    return 0;
}
