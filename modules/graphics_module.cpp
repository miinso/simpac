#include "include/graphics_module.h"

namespace graphics {

	graphics::graphics(flecs::world& ecs)
	{
		// Register module with world. The module entity will be created with the
		// same hierarchy as the C++ namespaces (e.g. simple::module)
		ecs.module<graphics>();

		// All contents of the module are created inside the module's namespace, so
		// the Position component will be created as simple::module::Position

		// Component registration is optional, however by registering components
		// inside the module constructor, they will be created inside the scope
		// of the module.
		ecs.component<Position>();
		ecs.component<Velocity>();

		ecs.system<Position, const Velocity>("Move").each([](Position& p, const Velocity& v) {
			p.x += v.x;
			p.y += v.y;
		});

		ecs.component<RaylibWindow>();
		ecs.component<RaylibCamera>();
		ecs.component<RaylibRenderer>();

		ecs.system<RaylibWindow>()
			.kind(flecs::OnStart)
			.each([](flecs::entity e, RaylibWindow& window) {
				InitWindow(window.width, window.height, window.title);
				SetTargetFPS(window.targetFPS);
			});

		ecs.system<RaylibWindow>()
			.kind(flecs::PostFrame)
			.each([](flecs::entity e, RaylibWindow& window) {
				EndMode3D();
				EndDrawing();
			});

		ecs.system<RaylibCamera>()
			.kind(flecs::OnStart)
			.each([](flecs::entity e, RaylibCamera& cam) {
				cam.camera.position = Vector3{5.0f, 5.0f, 5.0f};
				cam.camera.target = Vector3{0.0f, 0.0f, 0.0f};
				cam.camera.up = Vector3{0.0f, 1.0f, 0.0f};
				cam.camera.fovy = 60.0f;
				cam.camera.projection = CAMERA_PERSPECTIVE;
			});

		ecs.system<RaylibCamera>()
			.kind(flecs::PreUpdate)
			.each([](flecs::entity e, RaylibCamera& cam) {
				if(IsMouseButtonDown(MOUSE_LEFT_BUTTON))
				{
					UpdateCamera(&cam.camera, CAMERA_THIRD_PERSON);
				}
			});

		ecs.system<const RaylibRenderer, const RaylibCamera>()
			.kind(flecs::PreUpdate)
			.each([](flecs::entity e, const RaylibRenderer& renderer, const RaylibCamera& camera) {
				BeginDrawing();
				ClearBackground(renderer.backgroundColor);
				DrawFPS(10, 10);

				BeginMode3D(camera.camera);

				DrawSphere(Vector3{2, 0, 0}, 0.1, GREEN);
			});
	}

	void init_graphics(flecs::world& world, int width, int height, const char* title, int targetFPS)
	{
		world.entity()
			.set<RaylibWindow>({width, height, title, targetFPS})
			.set<RaylibRenderer>({
				BLUE, // backgroundColor
				5.0f, // particleRadius
				RED, // particleColor
				BLUE // constraintColor
			})
			.set<RaylibCamera>({});
		world.progress();
	}

} // namespace graphics