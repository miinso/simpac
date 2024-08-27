#ifndef GRAPHICS_MODULE_H
#define GRAPHICS_MODULE_H

#include "flecs.h"
#include "raylib.h"

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

	struct graphics {
		graphics(flecs::world& world); // Ctor that loads the module
	};

	// Free function to initialize graphics
	void
	init_graphics(flecs::world& world, int width, int height, const char* title, int targetFPS);

} // namespace graphics

#ifdef __cplusplus
}
#endif

#endif
