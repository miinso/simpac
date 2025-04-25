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
int num_substeps = 2;

Vector3r gravity (0, -9.81f, 0);

using namespace phys::pbd;
using namespace phys::pbd::rigidbody;


inline Vector3 e2r (const Vector3r& v) {
    return { (float)v.x (), (float)v.y (), (float)v.z () };
}

inline Vector3r e2r (const Vector3& v) {
    return { (Real)v.x, (Real)v.y, (Real)v.z };
}

int main () {
    flecs::world ecs;

    graphics::init (ecs);
    graphics::init_window (800, 600, "muller2007position");

    register_prefabs (ecs);
    ecs.set<Scene> ({ delta_time, 1, 1, gravity });

    auto rb1                        = add_rigid_body (ecs);
    rb1.get_mut<Position> ()->value = Vector3r (0, 1, 0);

    auto rb2 = add_rigid_body (ecs, { 1.5, 1, 0 });
    rb2.get_mut<AngularVelocity> ()->value = Vector3r (1, 0, 0);

    Constraint cc1;
    rb_mutual_orientation_constraint_init (cc1, rb1, rb2, 1e-2);
    ecs.entity ().set<Constraint> (cc1); // create anonymous entity with this constraint

    ////////// system registration
    ecs.system ("progress").kind (flecs::OnUpdate).run ([] (flecs::iter& it) {
        auto params = it.world ().get_mut<Scene> ();
        params->elapsed += params->timestep;
    });

    auto identify_island =
    ecs.system ("identify island").with<RigidBody> ().kind (0).run ([] (flecs::iter& it) {
        rb_identify_island (it, delta_time); // use full step
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
    .without<IsPinned> () // is fixex
    // .with<IsActive> ()
    .kind (0)
    .each ([&] (flecs::entity e, Position& x, Orientation& q, LinearVelocity& v,
           AngularVelocity& omega, OldPosition& x_old, OldOrientation& q_old,
           const LocalInertia& I_body, const LocalInverseInertia& I_body_inv,
           const LinearForce& f, const AngularForce& tau, const Mass mass) {
        rb_integrate (x.value, x_old.value, q.value, q_old.value, v.value,
        omega.value, I_body.value, I_body_inv.value, f.value, tau.value,
        mass.value, delta_time / num_substeps);
    });

    auto clear_lambda =
    ecs.system<Constraint> ("clear constraint lambda").kind (0).each ([&] (Constraint& c) {
        rb_clear_constraint_lambda (c);
    });

    auto collect_collision =
    ecs.system ("collect collision").with<RigidBody> ().kind (0).run ([] (flecs::iter& it) {
        rb_collect_collision (it);
    });

    auto solve_constraint =
    ecs.system<Constraint> ("constraint solve").kind (0).each ([&] (Constraint& c) {
        rb_solve_constraint (c, delta_time / num_substeps);
    });

    auto update_velocity =
    ecs
    .system<LinearVelocity, OldLinearVelocity, AngularVelocity, OldAngularVelocity,
    const Position, const OldPosition, const Orientation, const OldOrientation> ("update velocity")
    .with<RigidBody> ()
    .kind (0)
    .each ([&] (LinearVelocity& v, OldLinearVelocity& v_old, AngularVelocity& omega,
           OldAngularVelocity& omega_old, const Position& x, const OldPosition& x_old,
           const Orientation& q, const OldOrientation& q_old) {
        rb_update_velocity (v.value, v_old.value, omega.value, omega_old.value,
        x.value, x_old.value, q.value, q_old.value, delta_time / num_substeps);
    });

    ecs.system ("simulation loop").run ([&] (flecs::iter&) {
        auto sub_dt = delta_time / num_substeps;

        identify_island.run ();

        for (int i = 0; i < num_substeps; ++i) {
            clear_force.run ();
            // add_gravity.run ();
            compute_external_forces.run ();
            pbd_integrate.run ();

            clear_lambda.run ();

            // TODO: clear all existing contacts and
            // remove collision constraint

            // collect collision constraint
            // collect_collision.run ();

            for (int j = 0; j < num_iter; ++j) {
                solve_constraint.run ();
            }

            // handle_ground_collision.run();
            update_velocity.run ();

            // velocity dependent things
            // run_velocity_pass.run();
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
        aa.angle () * RAD2DEG, e2r (Vector3r::Ones ()), GREEN);
        DrawCircle3D (e2r (x.value), 0.2, e2r (aa.axis ()), aa.angle () * RAD2DEG, RED);
    });

    auto default_mat          = LoadMaterialDefault ();
    default_mat.maps->color.r = 0;
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

    ecs.system<Constraint> ("draw orientation constraint")
    .kind (graphics::OnRender)
    .each ([] (const Constraint& c) {
        auto e1 = c.e1;
        auto e2 = c.e2;
        auto x1 = e1.get<Position> ()->value;
        auto x2 = e2.get<Position> ()->value;
        // auto q1 = e1.get<Orientation> ()->value;
        // auto q2 = e2.get<Orientation> ()->value;

        float lambda_abs = std::abs (c.mutual_orientation_constraint.lambda);
        float t          = lambda_abs > 1e-10f ?
                 std::min (1.0f, -std::log10 (lambda_abs) / 10.0f) :
                 1.0f; // fully green when lambda is very close to 0

        Color color = ColorLerp (RED, GREEN, t);
        DrawCylinderEx (e2r (x1), e2r (x2), 0.01, 0.01, 5, color);
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
            rb2.get_mut<AngularVelocity> ()->value.x () +=
            0.1 * (IsKeyDown (KEY_LEFT_SHIFT) ? -1 : 1);
        }
        // it.fini();
    });

    graphics::run_main_loop ([] () { global_time += delta_time; });

    printf ("Simulation ended.\n");
    return 0;
}
