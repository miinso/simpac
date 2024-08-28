#include "include/graphics_module.h"

namespace graphics {

	flecs::world* graphics::s_world = nullptr;
	std::function<void()> graphics::s_update_func;

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

	void graphics::init_window(flecs::world& world, int width, int height, const char* title)
	{
		// world.entity().set<RaylibWindow>({width, height, title, 60});
		InitWindow(width, height, title);
	}

	// void graphics::update_draw_frame(flecs::world& world)
	// {
	// 	// world.progress();
	// 	BeginDrawing();

	// 	ClearBackground(RAYWHITE);
	// 	DrawFPS(20, 20);

	// 	DrawText("Whoop! You created your first window!", 190, 200, 20, LIGHTGRAY);

	// 	EndDrawing();
	// }

	void graphics::update_draw_frame2()
	{
		if(s_world)
		{
			// TODO: use correct delta
			s_world->progress();
		}

		BeginDrawing();

		ClearBackground(RAYWHITE);
		DrawFPS(20, 20);

		DrawText("Whoop! You've created your first window!", 190, 200, 20, LIGHTGRAY);

		EndDrawing();

		// run post hook here
		if(s_update_func)
		{
			s_update_func();
		}
	}

	bool graphics::window_should_close()
	{
		return WindowShouldClose();
	}

	void graphics::close_window()
	{
		return CloseWindow();
	}

	void graphics::main_loop_body(void* arg)
	{
		if(s_world && !window_should_close())
		{
			// run pre hook here

			// update_draw_frame(*s_world);
			update_draw_frame2();

			// run post hook here
			if(s_update_func)
			{
				s_update_func();
			}
		}
	}

	void graphics::run_main_loop(flecs::world& world, std::function<void()> update_func)
	{
		s_world = &world;
		s_update_func = update_func;

#if defined(__EMSCRIPTEN__)
		std::cout << "graphics::invoking emscripten rAF loop" << std::endl;
		// emscripten_set_main_loop_arg(main_loop_body, nullptr, 0, 1);
		emscripten_set_main_loop(update_draw_frame2, 0, 1);
		std::cout << "graphics::destroying emscripten rAF loop" << std::endl;
#else
		SetTargetFPS(60);
		std::cout << "graphics::invoking native while loop" << std::endl;
		while(!window_should_close())
		{
			main_loop_body(nullptr);
		}
		std::cout << "graphics::destroying native while loop" << std::endl;
#endif
		close_window();
	}
} // namespace graphics