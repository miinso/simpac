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

// TODO: window handling as a separate module. make it renderer agnostic

namespace graphics {
	class graphics
	{
	public:
		graphics(flecs::world& world);

		static void init_window(flecs::world& world, int width, int height, const char* title);
		static void run_main_loop(flecs::world& world, std::function<void()> update_func);
		static bool window_should_close();
		static void close_window();

		// static void begin_mode_3d();
		// static void end_mode_3d();
		// static Shader shader;
		static Camera3D camera;

	private:
		// static void main_loop_native(void* arg);
		// static void main_loop_rAF(void* arg);
		static void main_loop_rAF();
		static void main_loop_native();
		static flecs::world* s_world;
		static std::function<void()> s_update_func;
	};
} // namespace graphics

#ifdef __cplusplus
}
#endif

#endif
