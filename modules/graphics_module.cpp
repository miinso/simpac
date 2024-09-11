#include "include/graphics_module.h"

// #define RLIGHTS_IMPLEMENTATION
// #include "rlights.h"

#if defined(PLATFORM_DESKTOP)
#	define GLSL_VERSION 330
#else // PLATFORM_ANDROID, PLATFORM_WEB
#	define GLSL_VERSION 100
#endif

namespace graphics {

	flecs::world* graphics::s_world = nullptr;
	std::function<void()> graphics::s_update_func;
	Camera3D graphics::camera;

	graphics::graphics(flecs::world& ecs) {
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

		ecs.system("graphics::init_camera").kind(flecs::OnStart).run([](flecs::iter& it) {
			graphics::camera.position = Vector3{5.0f, 5.0f, 5.0f};
			graphics::camera.target = Vector3{0.0f, 0.0f, 0.0f};
			graphics::camera.up = Vector3{0.0f, 1.0f, 0.0f};
			graphics::camera.fovy = 60.0f;
			graphics::camera.projection = CAMERA_PERSPECTIVE;
		});

		// ecs.system("graphics::init_pbr_shader").kind(flecs::OnStart).run([](flecs::iter& it) {});

		ecs.system("graphics::update_camera").kind(flecs::PreUpdate).run([](flecs::iter& it) {
			// `q` for camera elevation down, `p` for camera elevation up
			UpdateCameraPro(
				&graphics::camera,
				Vector3{
					IsKeyDown(KEY_W) * 0.1f - // Move forward-backward
						IsKeyDown(KEY_S) * 0.1f,
					IsKeyDown(KEY_D) * 0.1f - // Move right-left
						IsKeyDown(KEY_A) * 0.1f,
					IsKeyDown(KEY_E) * 0.1f - IsKeyDown(KEY_Q) * 0.1f // Move up-down
				},
				Vector3{
					IsMouseButtonDown(MOUSE_LEFT_BUTTON) * (GetMouseDelta().x * 0.2f) +
						(IsKeyDown(KEY_RIGHT) * 1.5f - IsKeyDown(KEY_LEFT) * 1.5f), // yaw
					IsMouseButtonDown(MOUSE_LEFT_BUTTON) * (GetMouseDelta().y * 0.2f) +
						IsKeyDown(KEY_DOWN) * 1.5f - IsKeyDown(KEY_UP) * 1.5f, // pitch
					0.0f // Rotation: roll
				},
				GetMouseWheelMove() * -1.0f); // Move to target (zoom)
		});

		// ecs.system("graphics::begin").kind(flecs::OnUpdate).run([](flecs::iter& it) {
		// 	BeginDrawing();
		// });
		// ecs.system("graphics::clear").kind(flecs::OnUpdate).run([](flecs::iter& it) {
		// 	ClearBackground(RAYWHITE);
		// });
		// ecs.system("graphics::begin_mode_3d").kind(flecs::OnUpdate).run([](flecs::iter& it) {
		// 	BeginMode3D(graphics::camera);

		// 	DrawGrid(12, 10.0f / 12);
		// });

		// ecs.system("graphics::draw_particle_3d").kind(flecs::OnStore).run([this](flecs::iter& it) {
		// 	// query particles and draw
		// });

		// ecs.system("graphics::end_mode_3d").kind(flecs::OnStore).run([](flecs::iter& it) {
		// 	EndMode3D();

		// 	DrawFPS(20, 20);
		// });
		// ecs.system("graphics::end").kind(flecs::OnStore).run([](flecs::iter& it) { EndDrawing(); });
	}

	void graphics::init_window(flecs::world& world, int width, int height, const char* title) {
		// this somehow causes crash on unix
		// SetWindowState(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT | FLAG_WINDOW_HIGHDPI);

		InitWindow(width, height, title);
	}

	bool graphics::window_should_close() {
		return WindowShouldClose();
	}

	void graphics::close_window() {
		return CloseWindow();
	}

	void graphics::main_loop_native() {

		// flecs owns the main loop
		if(s_world) {
			// calling `progress` will invoke registered systems including
			// render pipeline. (clear->draw->swapchain) which is cool
			s_world->progress();
		}

		// run post hook here
		if(s_update_func) {
			s_update_func();
		}
	}

	void graphics::main_loop_rAF() {
		if(s_world) {
			s_world->progress();
		}

		if(s_update_func) {
			s_update_func();
		}
	}

	void graphics::run_main_loop(flecs::world& world, std::function<void()> update_func) {
		s_world = &world;
		s_update_func = update_func;

#if defined(__EMSCRIPTEN__)
		std::cout << "graphics::invoking emscripten rAF loop" << std::endl;
		emscripten_set_main_loop(main_loop_rAF, 0, 1);
		std::cout << "graphics::destroying emscripten rAF loop" << std::endl;
#else
		SetTargetFPS(60);
		std::cout << "graphics::invoking native while loop" << std::endl;
		while(!window_should_close()) {
			main_loop_native();
		}
		std::cout << "graphics::destroying native while loop" << std::endl;
#endif
		close_window();
	}
} // namespace graphics