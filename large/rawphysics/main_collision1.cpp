#include <flecs.h>

#include "graphics.h"

#include "entity.hpp"
#include "kernels.hpp"
#include "types.h"

#include <iostream>

float randf(int n)
{
	return static_cast<float>(rand() % n);
}

Real global_time  = 0.0;
Real delta_time   = 0.016;
int  num_iter     = 2;
int  num_substeps = 1;

Vector3r gravity(0, -9.81f, 0);

using namespace phys::pbd;
using namespace phys::pbd::rigidbody;

inline Vector3 e2r(const Vector3r &v)
{
	return {(float) v.x(), (float) v.y(), (float) v.z()};
}

inline Vector3r e2r(const Vector3 &v)
{
	return {(Real) v.x, (Real) v.y, (Real) v.z};
}

int main()
{
	flecs::world ecs;

	graphics::init(ecs);
	graphics::init_window(800, 600, "muller2007position");

	register_prefabs(ecs);
	ecs.set<Scene>({delta_time, 1, 1, gravity});

	// Flecs 4.1.1+: get_mut<T>() returns reference, not pointer
	auto rb1                      = add_box_object(ecs);
	rb1.get_mut<Position>().value = Vector3r(-1, 0, 0);

	auto rb2                             = add_box_object(ecs);
	rb2.get_mut<Position>().value        = Vector3r(1, 0.1, 0);
	rb2.get_mut<AngularVelocity>().value = Vector3r(0, 1, 1);
	rb2.get_mut<LinearVelocity>().value  = Vector3r(-0.1, 0, 0);

	////////// system registration
	ecs.system("progress").kind(flecs::OnUpdate).run([](flecs::iter &it) {
		auto& scene = it.world().get_mut<Scene>();
		scene.wall_time += it.delta_time();
		scene.sim_time += scene.dt;
		scene.frame_count++;
	});

	auto identify_island =
	    ecs.system("identify island")
	        .with<RigidBody>()
	        .kind(0)
	        .run([](flecs::iter &it) {
		        rb_identify_island(it, delta_time);        // use full step
	        });

	auto clear_force =
	    ecs.system<Forces, LinearForce, AngularForce>("clear force")
	        .with<RigidBody>()
	        .kind(0)        // `0` means it's gonna be called manually, see below
	        .each([](Forces &forces, LinearForce &f, AngularForce &tau) {
		        forces.a.clear();
		        rb_clear_force(f.value, tau.value);
	        });

	auto add_gravity = ecs.system<Forces, Mass>("add_gravity")
	                       .with<RigidBody>()
	                       .kind(0)
	                       .each([](Forces &forces, Mass &mass) {
		                       Physics_Force pf;
		                       Vector3r      gravity(0, -10, 0);
		                       pf.force    = gravity * mass.value;
		                       pf.position = Vector3r::Zero();
		                       forces.a.push_back(pf);
	                       });

	// compute forces. collect external forces or gravity
	auto compute_external_forces =
	    ecs.system<Forces, LinearForce, AngularForce>("compute external forces")
	        .with<RigidBody>()
	        .kind(0)
	        .each([](flecs::entity e, Forces &forces, LinearForce &f,
	                 AngularForce &tau) {
		        f.value   = rb_calculate_external_force(e);
		        tau.value = rb_calculate_external_torque(e);
	        });

	auto rigidbody_integrate =
	    ecs.system<Position, Orientation, LinearVelocity, AngularVelocity,
	               OldPosition, OldOrientation, const LocalInertia,
	               const LocalInverseInertia, const LinearForce,
	               const AngularForce, const Mass>("integrate")
	        .with<RigidBody>()
	        .without<IsPinned>()        // is fixex
	        // .with<IsActive> ()
	        .kind(0)
	        .each([&](flecs::entity e, Position &x, Orientation &q,
	                  LinearVelocity &v, AngularVelocity &omega,
	                  OldPosition &x_old, OldOrientation &q_old,
	                  const LocalInertia        &I_body,
	                  const LocalInverseInertia &I_body_inv, const LinearForce &f,
	                  const AngularForce &tau, const Mass mass) {
		        // if (e.get<IsActive> ()->value == true) {
		        rb_integrate(x.value, x_old.value, q.value, q_old.value, v.value,
		                     omega.value, I_body.value, I_body_inv.value, f.value,
		                     tau.value, mass.value, delta_time / num_substeps);
		        // }
	        });

	auto clear_lambda =
	    ecs.system<Constraint>("clear constraint lambda")
	        .kind(0)
	        .each([&](Constraint &c) { rb_clear_constraint_lambda(c); });

	auto clear_collision = ecs.system<Constraint>("clear collision")
	                           .kind(0)
	                           .each([](flecs::entity e, Constraint &c) {
		                           rb_clear_collision(e, c);
	                           });

	auto collect_collision =
	    ecs.system("collect collision")
	        .with<RigidBody>()
	        .kind(0)
	        .run([](flecs::iter &it) { rb_collect_collision(it); });

	auto solve_constraint =
	    ecs.system<Constraint>("constraint solve")
	        .kind(0)
	        .each([&](Constraint &c) {
		        rb_solve_constraint(c, delta_time / num_substeps);
	        });

	auto update_velocity =
	    ecs.system<LinearVelocity, OldLinearVelocity, AngularVelocity,
	               OldAngularVelocity, const Position, const OldPosition,
	               const Orientation, const OldOrientation>("update velocity")
	        .with<RigidBody>()
	        .kind(0)
	        .each([&](LinearVelocity &v, OldLinearVelocity &v_old,
	                  AngularVelocity &omega, OldAngularVelocity &omega_old,
	                  const Position &x, const OldPosition &x_old,
	                  const Orientation &q, const OldOrientation &q_old) {
		        rb_update_velocity(v.value, v_old.value, omega.value,
		                           omega_old.value, x.value, x_old.value, q.value,
		                           q_old.value, delta_time / num_substeps);
	        });

	ecs.system("simulation loop").run([&](flecs::iter &) {
		auto sub_dt = delta_time / num_substeps;

		identify_island.run();

		for (int i = 0; i < num_substeps; ++i)
		{
			clear_force.run();
			add_gravity.run();
			compute_external_forces.run();
			rigidbody_integrate.run();

			clear_lambda.run();

			// remove collision constraint
			clear_collision.run();

			// collect collision constraint
			collect_collision.run();

			for (int j = 0; j < num_iter; ++j)
			{
				solve_constraint.run();
			}

			update_velocity.run();

			// velocity dependent things
			// run_velocity_pass.run();
		}
	});

	// OnRender
	ecs.system<const Position, const Orientation>("draw rigidbody origin")
	    .with<RigidBody>()
	    .kind(graphics::phase_on_render)
	    .each([&](const Position &x, const Orientation &q) {
		    auto aa = Eigen::AngleAxisd(q.value);
		    DrawCircle3D(e2r(x.value), 0.2, e2r(aa.axis()), aa.angle() * RAD2DEG, RED);
	    });

	ecs.system<const Position, const Orientation, const RenderMesh>("draw rigidbody render mesh")
	    .with<RigidBody>()
	    .kind(graphics::phase_on_render)
	    .each([](flecs::entity e, const Position &x, const Orientation &q, const RenderMesh &render_mesh) {
		    auto aa = Eigen::AngleAxisd(q.value);
		    DrawModelWiresEx(render_mesh.model, e2r(x.value), e2r(aa.axis()), aa.angle() * RAD2DEG, e2r(Vector3r::Ones()), BLACK);
	    });

	ecs.system<const Position, const Orientation>("draw rigidbody frame")
	    .with<RigidBody>()
	    .kind(graphics::phase_on_render)
	    .each([](const Position &x, const Orientation &q) {
		    float scale  = 0.5;
		    auto  R      = q.value.toRotationMatrix();
		    auto  origin = x.value;

		    DrawLine3D(e2r(origin), e2r(R.col(0) * scale + origin), RED);          // x
		    DrawLine3D(e2r(origin), e2r(R.col(1) * scale + origin), GREEN);        // y
		    DrawLine3D(e2r(origin), e2r(R.col(2) * scale + origin), BLUE);         // z
	    });

	// Flecs 4.1.1+: get<T>() returns reference, not pointer
	ecs.system<Constraint>("draw contacts").kind(graphics::phase_on_render).each([](const Constraint &c) {
		if (c.type == phys::pbd::COLLISION_CONSTRAINT)
		{
			auto e1 = c.e1;
			auto e2 = c.e2;

			auto r1_wc = e1.get<Orientation>().value._transformVector(c.collision_constraint.r1_lc) + e1.get<Position>().value;
			auto r2_wc = e2.get<Orientation>().value._transformVector(c.collision_constraint.r2_lc) + e2.get<Position>().value;

			DrawSphere(e2r(r1_wc), 0.05, BLUE);
			DrawSphere(e2r(r2_wc), 0.05, PURPLE);
		}
	});

	ecs.system<const Position, const Orientation>("draw coordinate frame")
	    .with<RigidBody>()
	    .kind(graphics::phase_on_render)
	    .each([](const Position &x, const Orientation &q) {
		    // world uses y-up. xyz = rgb

		    float scale  = 0.5;
		    auto  R      = q.value.toRotationMatrix();
		    auto  origin = x.value;

		    DrawLine3D(e2r(origin), e2r(R.col(0) * scale + origin), RED);          // x
		    DrawLine3D(e2r(origin), e2r(R.col(1) * scale + origin), GREEN);        // y
		    DrawLine3D(e2r(origin), e2r(R.col(2) * scale + origin), BLUE);         // z
	    });

	// PostRender
	ecs.system("info").kind(graphics::phase_post_render).run([](flecs::iter &it) {
		const auto& scene = it.world().get<Scene>();

		Font font = graphics::get_font();
		char buffer[256];
		snprintf(buffer, sizeof(buffer),
			"Wall time: %.2fs  |  Sim time: %.2fs\n"
			"Frame: %d  |  Speed: %.2fx",
			scene.wall_time, scene.sim_time,
			scene.frame_count, scene.sim_time / (scene.wall_time + 1e-9f));
		DrawTextEx(font, buffer, {21, 41}, 12, 0, WHITE);
		DrawTextEx(font, buffer, {20, 40}, 12, 0, DARKGREEN);
	});

	// handle keyboard input
	ecs.system("key handle").kind(graphics::phase_pre_render).run([&](flecs::iter &it) {
		if (IsKeyDown(KEY_SPACE))
		{
			// throw_object (ecs);
		}
		// it.fini();
	});

	graphics::run_main_loop([]() { global_time += delta_time; });

	printf("Simulation has ended.\n");
	return 0;
}
