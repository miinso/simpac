#ifndef MODEL_MODULE_H
#define MODEL_MODULE_H

#include "flecs.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
we just can't raylib handle all the position and transform data. there has to be
a single source of an entire model state.
*/
namespace model {

	struct RaylibWindow {
		int width;
		int height;
		const char* title;
		int targetFPS;
	};

	struct model {
		model(flecs::world& world); // Ctor that loads the module
	};

	// Free function to initialize graphics
	void init_model(flecs::world& world);
    void add_particle();
    void add_spring();

} // namespace model

#ifdef __cplusplus
}
#endif

#endif
