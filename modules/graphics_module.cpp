#include "include/graphics_module.h"

namespace graphics {

	flecs::world* graphics::s_world = nullptr;
	std::function<void()> graphics::s_update_func;
	Camera3D graphics::camera;

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
		// ecs.component<Position>();
		// ecs.component<Velocity>();

		// ecs.system<Position, const Velocity>("Move").each([](Position& p, const Velocity& v) {
		// 	p.x += v.x;
		// 	p.y += v.y;
		// });

		// ecs.component<RaylibWindow>();
		// ecs.component<RaylibCamera>();
		// ecs.component<RaylibRenderer>();

		// ecs.system<RaylibWindow>()
		// 	.kind(flecs::OnStart)
		// 	.each([](flecs::entity e, RaylibWindow& window) {
		// 		InitWindow(window.width, window.height, window.title);
		// 		SetTargetFPS(window.targetFPS);
		// 	});

		// ecs.system<RaylibWindow>()
		// 	.kind(flecs::PostFrame)
		// 	.each([](flecs::entity e, RaylibWindow& window) {
		// 		EndMode3D();
		// 		EndDrawing();
		// 	});

		// ecs.system<RaylibCamera>()
		// 	.kind(flecs::OnStart)
		// 	.each([](flecs::entity e, RaylibCamera& cam) {
		// 		cam.camera.position = Vector3{5.0f, 5.0f, 5.0f};
		// 		cam.camera.target = Vector3{0.0f, 0.0f, 0.0f};
		// 		cam.camera.up = Vector3{0.0f, 1.0f, 0.0f};
		// 		cam.camera.fovy = 60.0f;
		// 		cam.camera.projection = CAMERA_PERSPECTIVE;
		// 	});

		// ecs.system<RaylibCamera>()
		// 	.kind(flecs::PreUpdate)
		// 	.each([](flecs::entity e, RaylibCamera& cam) {
		// 		if(IsMouseButtonDown(MOUSE_LEFT_BUTTON))
		// 		{
		// 			UpdateCamera(&cam.camera, CAMERA_THIRD_PERSON);
		// 		}
		// 	});

		// ecs.system<const RaylibRenderer, const RaylibCamera>()
		// 	.kind(flecs::PreUpdate)
		// 	.each([](flecs::entity e, const RaylibRenderer& renderer, const RaylibCamera& camera) {
		// 		BeginDrawing();
		// 		ClearBackground(renderer.backgroundColor);
		// 		DrawFPS(10, 10);

		// 		BeginMode3D(camera.camera);

		// 		DrawSphere(Vector3{2, 0, 0}, 0.1, GREEN);
		// 	});

		ecs.system("graphics::begin").kind(flecs::OnStore).run([](flecs::iter& it) {
			// begin, clear, begin3d, draw_particle, end3d, draw_ui, end
			BeginDrawing();

			ClearBackground(RAYWHITE);
			DrawFPS(20, 20);

			DrawText("Whoop! You've created your first window!", 190, 200, 20, LIGHTGRAY);

			char buf1[50];

			snprintf(buf1, 50, "%f", it.delta_time());
			DrawText(buf1, 10, 300, 20, RED);
			EndDrawing();
		});

		ecs.system("graphics::init_camera").kind(flecs::OnStart).run([](flecs::iter& it) {
			graphics::camera.position = Vector3{5.0f, 5.0f, 5.0f};
			graphics::camera.target = Vector3{0.0f, 0.0f, 0.0f};
			graphics::camera.up = Vector3{0.0f, 1.0f, 0.0f};
			graphics::camera.fovy = 60.0f;
			graphics::camera.projection = CAMERA_PERSPECTIVE;
		});

		ecs.system("graphics::update_camera").kind(flecs::PreUpdate).run([](flecs::iter& it) {
			// `q` for camera elevation down, `p` for camera elevation up
			UpdateCameraPro(
				&graphics::camera,
				(Vector3){
					IsKeyDown(KEY_W) * 0.1f - // Move forward-backward
						IsKeyDown(KEY_S) * 0.1f,
					IsKeyDown(KEY_D) * 0.1f - // Move right-left
						IsKeyDown(KEY_A) * 0.1f,
					IsKeyDown(KEY_E) * 0.1f - IsKeyDown(KEY_Q) * 0.1f // Move up-down
				},
				(Vector3){
					IsMouseButtonDown(MOUSE_LEFT_BUTTON) * (GetMouseDelta().x * 0.2f) +
						(IsKeyDown(KEY_RIGHT) * 1.5f - IsKeyDown(KEY_LEFT) * 1.5f), // yaw
					IsMouseButtonDown(MOUSE_LEFT_BUTTON) * (GetMouseDelta().y * 0.2f) +
						IsKeyDown(KEY_DOWN) * 1.5f - IsKeyDown(KEY_UP) * 1.5f, // pitch
					0.0f // Rotation: roll
				},
				GetMouseWheelMove() * 2.0f); // Move to target (zoom)
		});

		ecs.system("graphics::begin").kind(flecs::OnUpdate).run([](flecs::iter& it) {
			BeginDrawing();
		});
		ecs.system("graphics::clear").kind(flecs::OnUpdate).run([](flecs::iter& it) {
			ClearBackground(RAYWHITE);
			DrawFPS(20, 20);
		});
		ecs.system("graphics::begin_mode_3d").kind(flecs::OnUpdate).run([](flecs::iter& it) {
			BeginMode3D(graphics::camera);
			DrawGrid(10, 1.0f);
		});

		// ecs.system("graphics::draw_particle_3d").kind(flecs::OnStore).run([this](flecs::iter& it) {
		// 	// query particles and draw
		// });

		ecs.system("graphics::end_mode_3d").kind(flecs::OnStore).run([](flecs::iter& it) {
			EndMode3D();
		});
		ecs.system("graphics::end").kind(flecs::OnStore).run([](flecs::iter& it) { EndDrawing(); });
	}

	void init_graphics(flecs::world& world, int width, int height, const char* title, int targetFPS)
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
		// world.progress();
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

		// BeginDrawing();

		// ClearBackground(RAYWHITE);
		// DrawFPS(20, 20);

		// DrawText("Whoop! You've created your first window!", 190, 200, 20, LIGHTGRAY);

		// EndDrawing();

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

	void graphics::main_loop_native()
	{
		if(s_world && !window_should_close())
		{
			// flecs owns the main loop
			if(s_world)
			{
				// calling progress will invoke registered systems including
				// render loop. (clear-draw-swapchain)
				s_world->progress();
			}

			// run post hook here
			if(s_update_func)
			{
				s_update_func();
			}
		}
	}

	void graphics::main_loop_rAF()
	{
		if(s_world && !window_should_close())
		{
			if(s_world)
			{
				s_world->progress();
			}

			BeginDrawing();

			ClearBackground(RAYWHITE);
			DrawFPS(20, 20);

			DrawText(
				"emscripten::Whoop! You've created your first window!", 190, 200, 20, LIGHTGRAY);

			EndDrawing();

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
		std::cout << "yo6" << std::endl;
		// emscripten_set_main_loop_arg(main_loop_body, nullptr, 0, 1);
		// emscripten_set_main_loop(main_loop_rAF, 0, 1);
		emscripten_set_main_loop(update_draw_frame2, 0, 1);
		std::cout << "graphics::destroying emscripten rAF loop" << std::endl;
#else
		SetTargetFPS(60);
		std::cout << "graphics::invoking native while loop" << std::endl;
		while(!window_should_close())
		{
			main_loop_native();
		}
		std::cout << "graphics::destroying native while loop" << std::endl;
#endif
		close_window();
	}

	void graphics::begin_mode_3d()
	{
		BeginMode3D(graphics::camera);
	}

	void graphics::end_mode_3d()
	{
		EndMode3D();
	}
} // namespace graphics