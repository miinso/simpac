#include "flecs.h"
#include "graphics_module.h"

#include "Eigen/Dense"
#include <iostream>
#include <random>

const int CANVAS_WIDTH = 800;
const int CANVAS_HEIGHT = 600;

struct Scene {
	Eigen::Vector3f gravity;
	float air_resistence;

	float drawHZ;
	float timestep;
	float current_time;
};

struct Particle {
	Eigen::Vector3f position;
	Eigen::Vector3f velocity;
	Eigen::Vector3f position_old;
	Eigen::Vector3f velocity_old;

	float radius;
	float mass;
	float friction;
	float restitution;

	bool is_rest;
};

struct SphericalJoint {
	flecs::entity body1;
	flecs::entity body2;
	Eigen::Vector3f local_position1;
	Eigen::Vector3f local_position2;
};

struct DistanceConstraint {
	flecs::entity particle1;
	flecs::entity particle2;
	float rest;
	float stiffness;
};

flecs::world ecs;
Model sphere_model; // raylib Model
Model plane_model;
Texture2D texture0;

void solve_distance_constraint(flecs::iter& it,
							   size_t ss,
							   DistanceConstraint& constraint,
							   const Scene& scene)
{
	// it.world().defer_suspend();
	// waiter.add<Plate>(plate);
	// std::cout << ss << std::endl;
	auto world = it.world();

	auto p1 = constraint.particle1.get_mut<Particle>();
	auto p2 = constraint.particle2.get_mut<Particle>();

	auto diff = p1->position - p2->position;
	auto dist = diff.norm();
	auto grad_p1 = diff / dist; // dC
	auto grad_p2 = -diff / dist;

	float w1 = 1 / p1->mass;
	float w2 = 1 / p2->mass;
	float w = w1 + w2;

	float C = dist - constraint.rest; // C
	// float lambda = -violation / ((1 / p1->mass * direction.squaredNorm()) +
	// 							 1 / p2->mass * direction.squaredNorm());

	float lambda = -C / w;

	p1->position += (1 / p1->mass) * lambda * grad_p1;
	p2->position += (1 / p2->mass) * lambda * grad_p2;
	// p1->position = Eigen::Vector3f::Zero();
	// p2->position = Eigen::Vector3f::Zero();
	// it.world().defer_resume();
}

void update_velocity(flecs::iter& it, size_t, Particle& p, const Scene& scene)
{
	p.velocity = (p.position - p.position_old) * it.delta_time();
	// p.position_old = p.position;
	DrawModel(sphere_model, {p.position.x(), p.position.y(), p.position.z()}, p.radius, WHITE);
}

struct Plane {
	Plane()
	{
		position = Eigen::Vector3f::Zero();
		normal = Eigen::Vector3f::UnitY();
		friction = 0.5f;
		restitution = 0.5f;
	}
	Eigen::Vector3f position;
	Eigen::Vector3f normal;

	float friction;
	float restitution;
	float mass;
	// Eigen::Matrix3f R;
	// Eigen::Vector3f t;

	// PI/2 rotation along z axis
	// Eigen::Matrix3d R = Eigen::AngleAxisd(M_PI/2, Eigen::Vector3d(0, 0, 1)).toRotationMatrix();

	// d = (x - p).dot(n) + p.radius;
};

struct Contact {
	float time_of_impact;
	Eigen::Vector3f xl;
	Eigen::Vector3f nl;
	Eigen::Vector3f b, t; // tangent basis
	float d;
	// static float mu; // friction
};

void add_plane(Eigen::Vector3f position, Eigen::Vector3f normal)
{
	Plane plane;
	plane.position = position;
	plane.normal = normal;
	plane.friction = 1.0f;
	plane.restitution = 1.0f;
	ecs.entity().set<Plane>(plane);
}

flecs::entity add_particle(float x, float y, float z)
{
	Particle p;
	p.position = {x, y, z};
	p.velocity = {0, 0, 0};
	p.radius = 1;
	p.mass = 1;
	p.friction = 1;
	p.restitution = 0.5;

	auto e = ecs.entity();
	e.set<Particle>(p);

	// ecs.entity().set<Particle>(p);

	return e;
}

void integrate_particle(flecs::iter& it, size_t index, Particle& p, const Scene& scene)
{
	const float dt = it.delta_time();
	// const float dt = scene.timestep;

	// integrate
	p.position_old = p.position;
	p.velocity += dt * (1 / p.mass) * Eigen::Vector3f(0, -9, 0);
	p.position += dt * p.velocity;

	// ground collision check
	// if penetration is greater than zero, use that to flip the velocity
	const float pen = (p.position.y() - p.radius);
	if(pen < 0)
	{
		p.position.y() = -p.restitution * pen + p.radius;
		p.velocity.y() = -p.velocity.y();
	}
}

void draw_plane(flecs::iter& it, size_t, Plane& p)
{
	DrawModel(plane_model, {p.position.x(), p.position.y(), p.position.z()}, 1, WHITE);
	DrawModelEx(plane_model,
				{p.position.x(), p.position.y(), p.position.z()},
				{-p.normal.x(), -p.normal.y(), -p.normal.z()},
				-90.0f,
				{5, 5, 5},
				WHITE);
}

/*
detect collision (broad)

box, sphere, plane
box-sphere
plane-sphere


solve collision
*/

void test_collision(flecs::iter& it, size_t, Particle& p)
{
	// do double for loop and compare every possible collision pair
	// treat
}

int main(void)
{
	std::random_device rd;
	std::mt19937 e2(rd());
	std::uniform_real_distribution<float> distribution(-5.0, 5.0);

	std::cout << "hello from " << __FILE__ << std::endl;

	Scene scene;
	scene.gravity = -9.8 * Eigen::Vector3f::UnitY();
	scene.timestep = 1e-2;
	scene.drawHZ = 60;
	ecs.set<Scene>(scene); // scene as a global singleton

	ecs.import <graphics::graphics>();
	graphics::graphics::init_window(ecs, CANVAS_WIDTH, CANVAS_HEIGHT, "Simulator");
	ecs.progress();
	// ecs.import <model::model>();

	flecs::log::set_level(0);

	// Image checked = LoadImage("resources/uv2.png");
	Image checked = GenImageChecked(6, 6, 1, 1, LIGHTGRAY, DARKGRAY);
	texture0 = LoadTextureFromImage(checked);
	UnloadImage(checked);

	// GenMeshCubicmap(checked, Vector3{ 1.0f, 1.0f, 1.0f });
	// GenMeshCube(2.0f, 1.0f, 2.0f);

	plane_model = LoadModelFromMesh(GenMeshPlane(2, 2, 4, 3));
	plane_model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture0;

	sphere_model = LoadModelFromMesh(GenMeshSphere(1, 32, 32));
	sphere_model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture0;
	// add_particle(1, 5, 0);
	// add_particle(-1, 5, 0);

	add_plane(Eigen::Vector3f(5, 0, 0), Eigen::Vector3f::UnitX());

	flecs::entity last_particle;
	for(int i = 0; i < 5; ++i)
	{
		float x = distribution(e2);
		float z = distribution(e2);
		auto new_particle = add_particle(x, 5, z);

		if(last_particle.is_valid())
		{
			DistanceConstraint dc;
			dc.particle1 = last_particle;
			dc.particle2 = new_particle;
			dc.rest = 5.0f;
			dc.stiffness = 1e6;
			ecs.entity().set<DistanceConstraint>(dc);
		}

		last_particle = new_particle;
	}

	ecs.system<Particle, const Scene>("Integrator")
		.term_at(1)
		.singleton()
		.kind(flecs::OnUpdate)
		.each(integrate_particle);

	ecs.system<DistanceConstraint, const Scene>("SolveDistance")
		.term_at(1)
		.singleton()
		.kind(flecs::OnUpdate)
		.each(solve_distance_constraint);

	ecs.system<Particle, const Scene>("UpdateVelocity")
		.term_at(1)
		.singleton()
		.kind(flecs::OnUpdate)
		.each(update_velocity);

	ecs.system<Plane>("DrawPlane").kind(flecs::OnUpdate).each(draw_plane);

	// simulator should own the main loop, not renderer
	graphics::graphics::run_main_loop(ecs, []() {
		// std::cout << "additional main loop callback" << std::endl;
	});

	// code below will be executed once the main loop break
	std::cout << "Bye!" << std::endl;

	return 0;
}
