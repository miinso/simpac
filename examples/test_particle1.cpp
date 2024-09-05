#include "flecs.h"
#include "graphics_module.h"
#include "model_module.h"

#include "Eigen/Dense"
#include <iostream>
#include <random>
#include <sstream>

const int CANVAS_WIDTH = 800;
const int CANVAS_HEIGHT = 600;

struct Particle {
	Eigen::Vector3f position;
	Eigen::Vector3f velocity;
	float radius;
	float mass;
	float friction;
	float restitution;
};

struct Contact {
	Eigen::Vector3f xl;
	Eigen::Vector3f nl;
	Eigen::Vector3f b, t; // tangent basis
	float pen;
	// static float mu; // friction
};

flecs::world ecs;

void add_particle(float x, float y, float z)
{
	Particle p = {.position = {x, y, z},
				  .velocity = {0, 0, 0},
				  .radius = 1,
				  .mass = 1,
				  .friction = 1,
				  .restitution = 1};
	ecs.entity().set<Particle>(p);
}

void integrate_particle(flecs::iter& it, size_t, Particle& p)
{
	const float dt = it.delta_time();

	// integrate
	p.velocity += dt * (1 / p.mass) * Eigen::Vector3f(0, -9.8, 0);
	p.position += dt * p.velocity;

	// ground collision check
	// if penetration is greater than zero, use that to flip the velocity
	const float pen = (p.position.y() - p.radius);
	if(pen < 0)
	{
		p.velocity.y() = -p.velocity.y();
	}

	// graphics::graphics::begin_mode_3d();
	DrawSphere(Vector3{p.position.x(), p.position.y(), p.position.z()}, p.radius, GRAY);
	DrawSphereWires(
		Vector3{p.position.x(), p.position.y(), p.position.z()}, p.radius, 8, 8, DARKGRAY);
	// graphics::graphics::end_mode_3d();
}

void test_collision() { }

int main(void)
{
	std::random_device rd;
	std::mt19937 e2(rd());
	std::uniform_real_distribution<float> distribution(-10.0, 10.0);

	std::cout << "hello from " << __FILE__ << std::endl;

	ecs.import <graphics::graphics>();
	// ecs.import <model::model>();

	flecs::log::set_level(0);

	// add_particle(1, 5, 0);
	// add_particle(-1, 5, 0);

	for(int i = 0; i < 1000; ++i)
	{
		float x = distribution(e2);
		float z = distribution(e2);
		add_particle(x, 5, z);
	}

	ecs.system<Particle>("Integrator").kind(flecs::OnUpdate).each(integrate_particle);

	graphics::graphics::init_window(ecs, CANVAS_WIDTH, CANVAS_HEIGHT, "Simulator");

	graphics::graphics::run_main_loop(ecs, []() {
		// std::cout << "additional main loop callback" << std::endl;
	});

	// code below will be executed once the main loop break
	std::cout << "Bye!" << std::endl;

	return 0;
}
