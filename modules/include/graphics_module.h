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

	// struct Position {
	// 	double x, y;
	// };

	// struct Velocity {
	// 	double x, y;
	// };

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
		static void main_loop_rAF();

		static void begin_mode_3d();
		static void end_mode_3d();

	private:
		// static void main_loop_native(void* arg);
		// static void main_loop_rAF(void* arg);
		static void main_loop_native();
		static flecs::world* s_world;
		static std::function<void()> s_update_func;

		static Camera3D camera;
	};
} // namespace graphics

#ifdef __cplusplus
}
#endif

#endif
