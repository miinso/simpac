#ifndef GRAPHICS_MODULE_H
#define GRAPHICS_MODULE_H

#include "flecs.h"
#include "raylib.h"

#include <functional>
#include <iostream>

#if defined(__EMSCRIPTEN__)
#	include <emscripten/emscripten.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

namespace graphics {

	struct Position {
		double x, y;
	};

	struct Velocity {
		double x, y;
	};

	struct RaylibWindow {
		int width;
		int height;
		const char* title;
		int targetFPS;
	};

	struct RaylibRenderer {
		Color backgroundColor;
		float particleRadius;
		Color particleColor;
		Color constraintColor;
	};

	struct RaylibCamera {
		Camera3D camera;
	};

	// struct graphics {
	// 	graphics(flecs::world& world); // Ctor that loads the module
	// };

	// // Free function to initialize graphics
	// void
	// init_graphics(flecs::world& world, int width, int height, const char* title, int targetFPS);

	// void init_window(flecs::world& world, int width, int height, const char* title);

	// bool window_should_close();
	// void update_draw_frame();
	// void close_window();
	class graphics
	{
	public:
		graphics(flecs::world& world);

		static void init_window(flecs::world& world, int width, int height, const char* title);
		static bool window_should_close();
		static void update_draw_frame(flecs::world& world);
		static void update_draw_frame2();
		static void close_window();
		static void run_main_loop(flecs::world& world, std::function<void()> update_func);

	private:
		static void main_loop_body(void* arg);
		static flecs::world* s_world;
		static std::function<void()> s_update_func;
	};
} // namespace graphics

#ifdef __cplusplus
}
#endif

#endif
