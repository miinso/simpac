#include "flecs.h"
#include "graphics_module.h"
#include "model_module.h"

#include "Eigen/Dense"
#include "raylib.h"
#include <iostream>

#if defined(PLATFORM_WEB)
#	include <emscripten/emscripten.h>
#endif

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

void UpdateDrawFrame(void);

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
	std::cout << "hello from raylib_window.cpp" << std::endl;
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

	flecs::log::set_level(1);

	Wow wow;
	wow.x << 1, 2, 3;
	std::cout << "eigen vector3f: " << wow.x.transpose() << std::endl;

	// Create a few test entities for a Position, Velocity query
	ecs.entity("e1").set<Position>({10, 20}).set<Velocity>({1, 2});
	ecs.entity("e2").set<Position>({10, 20}).set<Velocity>({3, 4});

	ecs.system<Position, const Velocity>("integrate")
		.kind(flecs::OnUpdate)
		.each(integrate_particle);

	InitWindow(screenWidth, screenHeight, "raylib [core] example - basic window");

#if defined(PLATFORM_WEB)
	std::cout << "running emscripten rAF loop" << std::endl;
	emscripten_set_main_loop(UpdateDrawFrame, 0, 1);
#else
	std::cout << "running native while loop" << std::endl;
	SetTargetFPS(60);

	while(!WindowShouldClose())
	{
		UpdateDrawFrame();
	}
#endif

	CloseWindow();

	return 0;
}

void UpdateDrawFrame(void)
{
	BeginDrawing();

	ClearBackground(RAYWHITE);
	ecs.progress();
	DrawFPS(20, 20);

	DrawText("Whoop! You created your first window!", 190, 200, 20, LIGHTGRAY);

	EndDrawing();
}