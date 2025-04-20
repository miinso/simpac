#include <flecs.h>

#include "graphics.h"

#include "kernels.hpp"
#include "types.hpp"

float randf (int n) {
    return static_cast<float> (rand () % n);
}

Real global_time = 0.0;
Real delta_time  = 0.016;
int num_iter     = 10;
int num_substeps = 100;

Vector3r gravity (0, -9.81f, 0);

using namespace phys::pbd;
using namespace phys::pbd::rigidbody;

namespace prefabs {
struct Particle {};
struct RigidBody {};
} // namespace prefabs

// should i use
//
// struct Position {
//     Vector3r value;
//     Vector3r prev;
//     Vector3r init;
// };
//
// instead of OldPosition?

void register_prefabs (flecs::world& ecs) {

    ecs.prefab<prefabs::RigidBody> ()
    .add<RigidBody> ()

    .add<Position> ()
    .add<Orientation> ()
    .add<LinearVelocity> ()
    .add<AngularVelocity> ()

    .add<Position0> ()
    .add<Orientation0> ()
    .add<LinearVelocity0> ()
    .add<AngularVelocity0> ()

    .add<OldPosition> ()
    .add<OldOrientation> ()
    .add<OldLinearVelocity> ()
    .add<OldAngularVelocity> ()

    .add<LinearForce> ()
    .add<AngularForce> () // torque
    .add<Mass> ()
    .add<InverseMass> ()
    .add<LocalInertia> ()
    .add<LocalInverseInertia> ()
    .add<WorldInertia> ()
    .add<WorldInverseInertia> ()

    // .add<PhysicsMesh>();
    .add<Forces> ();
}

flecs::entity add_rigid_body (flecs::world& ecs,
const Vector3r& x0      = Vector3r::Zero (),
const Quaternionr& q0   = Quaternionr::Identity (),
const Vector3r& v0      = Vector3r::Zero (),
const Vector3r& omega0  = Vector3r::Zero (),
Real mass               = 1.0f,
const Matrix3r& inertia = Matrix3r::Identity ()) {

    // TODO: inertia should be set when a physics_mesh is attached to this
    // entity do closed form thing or volume integration.
    Matrix3r inv_inertia = inertia.inverse ();

    return ecs.entity ()
    .is_a<prefabs::RigidBody> ()

    .set<Position> ({ x0 })
    .set<Orientation> ({ q0 })
    .set<LinearVelocity> ({ v0 })
    .set<AngularVelocity> ({ omega0 })

    .set<Position0> ({ x0 })
    .set<Orientation0> ({ q0 })
    .set<LinearVelocity0> ({ v0 })
    .set<AngularVelocity0> ({ omega0 })

    .set<OldPosition> ({ x0 })
    .set<OldOrientation> ({ q0 })
    .set<OldLinearVelocity> ({ v0 })
    .set<OldAngularVelocity> ({ omega0 })

    .set<LinearForce> ({ Vector3r::Zero () })
    .set<AngularForce> ({ Vector3r::Zero () })
    .set<Mass> ({ mass })
    .set<InverseMass> ({ 1.0f / mass })
    .set<LocalInertia> ({ inertia })
    .set<LocalInverseInertia> ({ inv_inertia })
    .set<WorldInertia> ({ inertia })
    .set<WorldInverseInertia> ({ inv_inertia });
}

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

    auto rb1                              = add_rigid_body (ecs);
    auto rb2                              = add_rigid_body (ecs, { 0, 3, 0 });
    // rb1.get_mut<LinearVelocity> ()->value = Vector3r (0, 10, 0);
    // rb2.get_mut<LinearVelocity> ()->value = Vector3r (0, 1, 0);
    // rb2.add<IsPinned> ();
    // rb2.get_mut<Position> ()->value = Vector3r (0, 5, 0);

    // PositionalConstraint aa;
    // Constraint constraint{ .type = phys::pbd::POSITIONAL_CONSTRAINT, .e1 = rb1, .e2 = rb2 };
    // constraint.type = phys::pbd::POSITIONAL_CONSTRAINT;
    // pbd_positional_constraint_init(&constraint, rb1, attachment_eid, (vec3) {
    // 0.0, 0.0, 0.0 }, (vec3) { 0.0, 0.0, 0.0 }, 0.001, (vec3) { 0.0, -3.0, 0.0
    // }); array_push(constraints, constraint);

    // p1.add<IsPinned>();
    // p1.set<InverseMass>({0});

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
        Vector3r gravity (0, -1, 0);
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

        rb_integrate (x.value, q.value, v.value, omega.value, I_body.value,
        I_body_inv.value, f.value, tau.value, mass.value, delta_time / num_substeps);
    });

    auto clear_lambda =
    ecs.system<Constraint> ("clear constraint lambda").kind (0).each ([&] (Constraint& c) {
        rb_clear_constraint_lambda (c);
    });

    auto solve_constraint =
    ecs.system<Constraint> ("constraint solve").kind (0).each ([&] (Constraint& c) {
        // particle_solve_constraint(c, delta_time / num_substeps);
        // auto e1 = c.e1;
        // auto e2 = c.e2;

        // auto x1 = e1.get_mut<Position>();
        // auto x2 = e2.get_mut<Position>();

        // auto w1 = e1.get<InverseMass>();
        // auto w2 = e2.get<InverseMass>();
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
    .system<LinearVelocity, AngularVelocity, const Position, const OldPosition, const Orientation, const OldOrientation> (
    "update velocity")
    .with<RigidBody> ()
    .kind (0)
    .each ([&] (LinearVelocity& v, AngularVelocity& omega, const Position& x,
           const OldPosition& x_old, const Orientation& q, const OldOrientation& q_old) {
        // particle_update_velocity(v.value, x.value, x_old.value, delta_time / num_substeps);
        rb_update_velocity (v.value, omega.value, x.value, x_old.value, q.value,
        q_old.value, delta_time / num_substeps);
    });

    ecs.system ("simulation loop").run ([&] (flecs::iter&) {
        auto sub_dt = delta_time / num_substeps;

        for (int i = 0; i < num_substeps; ++i) {
            // clear_force.run ();
            // add_gravity.run ();
            // compute_external_forces.run ();
            pbd_integrate.run ();

            clear_lambda.run ();

            // for (int j = 0; j < num_iter; ++j) {
            //     solve_constraint.run ();
            // }

            // handle_ground_collision.run();
            // update_velocity.run ();
        }
    });

    //////// drawings below

    auto mmm = LoadModelFromMesh (GenMeshCube (1, 1, 1));

    ecs.system<const Position, const Orientation> ("draw_physics_mesh")
    .with<RigidBody> ()
    .kind (graphics::OnRender)
    .each ([&] (const Position& x, const Orientation q) {
        // draw physics mesh
        // DrawCube(e2r(x.value), q.value.w(), 1, 1, BLUE);
        // auto m = body.physics_mesh;
        // DrawMesh(x, q, m);

        auto angle = 2.0 * acos (q.value.w ()); // radian
        auto s     = sqrt (1.0f - q.value.w () * q.value.w ());
        auto axis = Vector3r (q.value.x () / s, q.value.y () / s, q.value.z () / s);

        DrawModelEx (mmm, e2r (x.value), e2r (axis), angle * RAD2DEG, { 1, 1, 1 }, BLUE);
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
    .with<Particle> ()
    .kind (graphics::PostRender)
    .each ([] (const Position& x, const Mass& m) {
        Vector3 textPos = e2r (x.value);
        textPos.y += 0.2f;
        Vector2 screenPos = GetWorldToScreen (textPos, graphics::camera);

        char id_buffer[32];

        const char* label = (sprintf (id_buffer, "%0.2f", m.value), id_buffer);

        DrawText (label, screenPos.x, screenPos.y, 20, BLUE);
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

    // ecs.import <flecs::stats>(); % this causes problems on web build
    // ecs.set<flecs::Rest>({});

    graphics::run_main_loop ([] () { global_time += delta_time; });

    printf ("Simulation ended.\n");
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