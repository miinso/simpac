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

	// struct RaylibWindow {
	// 	int width;
	// 	int height;
	// 	const char* title;
	// 	int targetFPS;
	// };

	struct model {
		model(flecs::world& world); // Ctor that loads the module
	};

	// Free function to initialize graphics
	void init_model(flecs::world& world);
    void add_particle();
    void add_spring();

	// register analytic shapes
	// Model sphere = sphere_model = LoadModelFromMesh(GenMeshSphere(1, 32, 32));
	// sphere_model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture0;
	
} // namespace model

#ifdef __cplusplus
}
#endif

#endif
