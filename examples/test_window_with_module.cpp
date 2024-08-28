#include "flecs.h"
#include "graphics_module.h"
#include "model_module.h"

#include "Eigen/Dense"
#include <iostream>

const int screenWidth = 800;
const int screenHeight = 450;

struct Position {
	double x, y;
};

struct Velocity {
	double x, y;
};

struct Wow {
	Eigen::Vector3f x;
};

void integrate_particle(flecs::iter& it, size_t, Position& p, const Velocity& v)
{
	p.x += v.x;
	p.y += v.y;

	// std::cout << ": {" << p.x << ", " << p.y << "}\n";

	char posText[50];
	sprintf(posText, "Position: {%f, %f}", p.x, p.y);
	char velText[50];
	sprintf(velText, "Velocity: {%f, %f}", v.x, v.y);

	DrawText(posText, 10, 10, 20, BLUE);
	DrawText(velText, 10, 40, 20, BLUE);
}

flecs::world ecs;

int main(void)
{
	std::cout << "hello from " << __FILE__ << std::endl;

#ifdef PLATFORM_WEB
	std::cout << "PLATFORM_WEB is defined" << std::endl;
#else
	std::cout << "PLATFORM_WEB is not defined" << std::endl;
#endif

#ifdef __EMSCRIPTEN__
	std::cout << "__EMSCRIPTEN__ is defined" << std::endl;
#else
	std::cout << "__EMSCRIPTEN__ is not defined" << std::endl;
#endif

	ecs.import <graphics::graphics>();
	ecs.import <model::model>();

	flecs::log::set_level(0);

	Wow wow;
	wow.x << 1, 2, 3;
	std::cout << "eigen vector3f: " << wow.x.transpose() << std::endl;

	ecs.entity("e1").set<Position>({10, 20}).set<Velocity>({1, 2});
	// ecs.entity("e2").set<Position>({10, 20}).set<Velocity>({3, 4});

	ecs.system<Position, const Velocity>("integrator")
		.kind(flecs::OnUpdate)
		.each(integrate_particle);

	graphics::graphics::init_window(
		ecs, screenWidth, screenHeight, "raylib example - window from module");

	graphics::graphics::run_main_loop(ecs, []() {
		// std::cout << "additional main loop callback" << std::endl;
	});

	// code below will be executed once the main loop break
	std::cout << "bye!" << std::endl;

	return 0;
}
