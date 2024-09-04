#include "flecs.h"
#include "graphics_module.h"
#include "model_module.h"

#include "Eigen/Dense"
#include <iostream>
#include <sstream>

const int screenWidth = 800;
const int screenHeight = 450;

struct Position {
	double x, y, z;
	// Eigen::Vector3f value;
};

struct Velocity {
	double x, y, z;
	// Eigen::Vector3f value;
};

struct Particle {
	Eigen::Vector3f position;
	Eigen::Vector3f velocity;
	float radius;
	float mass;
	float friction;
};

struct Contact {
	Eigen::Vector3f xl;
	Eigen::Vector3f nl;
	Eigen::Vector3f b, t; // tangent basis
	float pen;
	// static float mu; // friction
};

struct Wow {
	Eigen::Vector3f x;
};

flecs::world ecs;

void add_particle()
{
	ecs.entity().set<Particle>({{1, 1, 1}, {0.121, 0.023, 0.717}, 1, 1, 1});
}

void integrate_particle(flecs::iter& it, size_t, Position& p, const Velocity& v)
{
	p.x += v.x;
	p.y += v.y;
	p.z += v.z;
	// std::cout << ": {" << p.x << ", " << p.y << "}\n";

	char posText[50];
	sprintf(posText, "Position: {%.3f, %.3f, %.3f}", p.x, p.y, p.z);
	char velText[50];
	sprintf(velText, "Velocity: {%.3f, %.3f, %.3f}", v.x, v.y, v.z);

	DrawText(posText, 10, 10, 20, BLUE);
	DrawText(velText, 10, 40, 20, BLUE);
}

void integrate_particle2(flecs::iter& it, size_t, Particle& p)
{
	p.position += p.velocity;
	// p.position += dt * p.velocity;

	// std::cout << p.position.x() << p.position.y() << p.position.z() << std::endl;

	std::ostringstream posStream, velStream;
	posStream << "Position: {" << p.position.x() << ", " << p.position.y() << ", " << p.position.z()
			  << "}";
	velStream << "Velocity: {" << p.velocity.x() << ", " << p.velocity.y() << ", " << p.velocity.z()
			  << "}";

	DrawText(posStream.str().c_str(), 10, 110, 20, BLUE);
	DrawText(velStream.str().c_str(), 10, 140, 20, BLUE);
}

void test_collision() { }

int main(void)
{
	std::cout << "hello from " << __FILE__ << std::endl;

// #ifdef PLATFORM_WEB
// 	std::cout << "PLATFORM_WEB is defined" << std::endl;
// #else
// 	std::cout << "PLATFORM_WEB is not defined" << std::endl;
// #endif

// #ifdef __EMSCRIPTEN__
// 	std::cout << "__EMSCRIPTEN__ is defined" << std::endl;
// #else
// 	std::cout << "__EMSCRIPTEN__ is not defined" << std::endl;
// #endif

	ecs.import <graphics::graphics>();
	// ecs.import <model::model>();

	flecs::log::set_level(0);

	Wow wow;
	wow.x << 1, 2, 3;
	std::cout << "eigen vector3f: " << wow.x.transpose() << std::endl;

	ecs.entity("e1").set<Position>({10, 20}).set<Velocity>({0.117, 0.09367});
	// ecs.entity("e2").set<Position>({10, 20}).set<Velocity>({3, 4});

	add_particle();

	ecs.system<Position, const Velocity>("integrator")
		.kind(flecs::OnUpdate)
		.each(integrate_particle);

	ecs.system<Particle>("integrator2").kind(flecs::OnUpdate).each(integrate_particle2);

	graphics::graphics::init_window(ecs, screenWidth, screenHeight, "particle test");

	graphics::graphics::run_main_loop(ecs, []() {
		// std::cout << "additional main loop callback" << std::endl;
	});

	// code below will be executed once the main loop break
	std::cout << "bye!" << std::endl;

	return 0;
}
