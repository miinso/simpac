#include "include/model_module.h"

namespace model {

	model::model(flecs::world& ecs)
	{
		ecs.module<model>();

		// ecs.component<Position>();
		// ecs.component<Velocity>();

		// ecs.system<Position, const Velocity>("Move").each([](Position& p, const Velocity& v) {
		// 	p.x += v.x;
		// 	p.y += v.y;
		// });

		// ecs.system<const RaylibRenderer, const RaylibCamera>()
		// 	.kind(flecs::PreUpdate)
		// 	.each([](flecs::entity e, const RaylibRenderer& renderer, const RaylibCamera& camera) {
		// 		BeginDrawing();
		// 		ClearBackground(renderer.backgroundColor);
		// 		DrawFPS(10, 10);

		// 		BeginMode3D(camera.camera);

		// 		DrawSphere(Vector3{2, 0, 0}, 0.1, GREEN);
		// 	});
	}

	void init_model(flecs::world& world)
	{
		// world.entity()
		// 	.set<RaylibWindow>({width, height, title, targetFPS})
		// 	.set<RaylibRenderer>({
		// 		BLUE, // backgroundColor
		// 		5.0f, // particleRadius
		// 		RED, // particleColor
		// 		BLUE // constraintColor
		// 	})
		// 	.set<RaylibCamera>({});
		world.progress();
	}

	void add_particle() { }
	void add_spring() { }
} // namespace model