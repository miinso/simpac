#include "include/graphics_module.h"

#define RLIGHTS_IMPLEMENTATION
#include "rlights.h"

#if defined(PLATFORM_DESKTOP)
#	define GLSL_VERSION 330
#else // PLATFORM_ANDROID, PLATFORM_WEB
#	define GLSL_VERSION 100
#endif

#define MAX_LIGHTS 4

namespace graphics {

	// Light type
	typedef enum
	{
		LIGHT_DIRECTIONAL = 0,
		LIGHT_POINT,
		LIGHT_SPOT
	} LightType;

	// Light data
	typedef struct {
		int type;
		int enabled;
		Vector3 position;
		Vector3 target;
		float color[4];
		float intensity;

		// Shader light parameters locations
		int typeLoc;
		int enabledLoc;
		int positionLoc;
		int targetLoc;
		int colorLoc;
		int intensityLoc;
	} Light;
	//----------------------------------------------------------------------------------
	// Global Variables Definition
	//----------------------------------------------------------------------------------
	static int lightCount = 0; // Current number of dynamic lights that have been created

	//----------------------------------------------------------------------------------
	// Module specific Functions Declaration
	//----------------------------------------------------------------------------------
	// Create a light and get shader locations
	static Light CreateLight(
		int type, Vector3 position, Vector3 target, Color color, float intensity, Shader shader);

	// Update light properties on shader
	// NOTE: Light shader locations should be available
	static void UpdateLight(Shader shader, Light light);

	flecs::world* graphics::s_world = nullptr;
	std::function<void()> graphics::s_update_func;
	Camera3D graphics::camera;
	Shader graphics::shader;
	Light lights[MAX_LIGHTS] = {0};

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

		ecs.system("graphics::init_camera").kind(flecs::OnStart).run([](flecs::iter& it) {
			graphics::camera.position = Vector3{5.0f, 5.0f, 5.0f};
			graphics::camera.target = Vector3{0.0f, 0.0f, 0.0f};
			graphics::camera.up = Vector3{0.0f, 1.0f, 0.0f};
			graphics::camera.fovy = 60.0f;
			graphics::camera.projection = CAMERA_PERSPECTIVE;
		});

		ecs.system("graphics::init_pbr_shader").kind(flecs::OnStart).run([](flecs::iter& it) {
			// // Load PBR shader and setup all required locations
			// shader = LoadShader(TextFormat("resources/shaders/glsl%i/pbr.vs", GLSL_VERSION),
			// 					TextFormat("resources/shaders/glsl%i/pbr.fs", GLSL_VERSION));
			// shader.locs[SHADER_LOC_MAP_ALBEDO] = GetShaderLocation(shader, "albedoMap");
			// // WARNING: Metalness, roughness, and ambient occlusion are all packed into a MRA texture
			// // They are passed as to the SHADER_LOC_MAP_METALNESS location for convenience,
			// // shader already takes care of it accordingly
			// shader.locs[SHADER_LOC_MAP_METALNESS] = GetShaderLocation(shader, "mraMap");
			// shader.locs[SHADER_LOC_MAP_NORMAL] = GetShaderLocation(shader, "normalMap");
			// // WARNING: Similar to the MRA map, the emissive map packs different information
			// // into a single texture: it stores height and emission data
			// // It is binded to SHADER_LOC_MAP_EMISSION location an properly processed on shader
			// shader.locs[SHADER_LOC_MAP_EMISSION] = GetShaderLocation(shader, "emissiveMap");
			// shader.locs[SHADER_LOC_COLOR_DIFFUSE] = GetShaderLocation(shader, "albedoColor");

			// // Setup additional required shader locations, including lights data
			// shader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(shader, "viewPos");
			// int lightCountLoc = GetShaderLocation(shader, "numOfLights");
			// int maxLightCount = MAX_LIGHTS;
			// SetShaderValue(shader, lightCountLoc, &maxLightCount, SHADER_UNIFORM_INT);

			// // Setup ambient color and intensity parameters
			// float ambientIntensity = 0.02f;
			// Color ambientColor = Color{26, 32, 135, 255};
			// Vector3 ambientColorNormalized =
			// 	Vector3{ambientColor.r / 255.0f, ambientColor.g / 255.0f, ambientColor.b / 255.0f};
			// SetShaderValue(shader,
			// 			   GetShaderLocation(shader, "ambientColor"),
			// 			   &ambientColorNormalized,
			// 			   SHADER_UNIFORM_VEC3);
			// SetShaderValue(shader,
			// 			   GetShaderLocation(shader, "ambient"),
			// 			   &ambientIntensity,
			// 			   SHADER_UNIFORM_FLOAT);

			// // Get location for shader parameters that can be modified in real time
			// // int emissiveIntensityLoc = GetShaderLocation(shader, "emissivePower");
			// // int emissiveColorLoc = GetShaderLocation(shader, "emissiveColor");
			// // int textureTilingLoc = GetShaderLocation(shader, "tiling");

			// ///

			// // Create some lights

			// lights[0] = CreateLight(LIGHT_POINT,
			// 						Vector3{-1.0f, 1.0f, -2.0f},
			// 						Vector3{0.0f, 0.0f, 0.0f},
			// 						YELLOW,
			// 						4.0f,
			// 						shader);
			// lights[1] = CreateLight(LIGHT_POINT,
			// 						Vector3{2.0f, 1.0f, 1.0f},
			// 						Vector3{0.0f, 0.0f, 0.0f},
			// 						GREEN,
			// 						3.3f,
			// 						shader);
			// lights[2] = CreateLight(LIGHT_POINT,
			// 						Vector3{-2.0f, 1.0f, 1.0f},
			// 						Vector3{0.0f, 0.0f, 0.0f},
			// 						RED,
			// 						8.3f,
			// 						shader);
			// lights[3] = CreateLight(LIGHT_POINT,
			// 						Vector3{1.0f, 1.0f, -2.0f},
			// 						Vector3{0.0f, 0.0f, 0.0f},
			// 						BLUE,
			// 						2.0f,
			// 						shader);

			// // Setup material texture maps usage in shader
			// // NOTE: By default, the texture maps are always used
			// int usage = 1;
			// SetShaderValue(
			// 	shader, GetShaderLocation(shader, "useTexAlbedo"), &usage, SHADER_UNIFORM_INT);
			// SetShaderValue(
			// 	shader, GetShaderLocation(shader, "useTexNormal"), &usage, SHADER_UNIFORM_INT);
			// SetShaderValue(
			// 	shader, GetShaderLocation(shader, "useTexMRA"), &usage, SHADER_UNIFORM_INT);
		});

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

		ecs.system("graphics::begin").kind(flecs::OnUpdate).run([](flecs::iter& it) {
			// float cameraPos[3] = {camera.position.x, camera.position.y, camera.position.z};
			// SetShaderValue(
			// 	shader, shader.locs[SHADER_LOC_VECTOR_VIEW], cameraPos, SHADER_UNIFORM_VEC3);

			// for(int i = 0; i < MAX_LIGHTS; i++)
			// 	UpdateLight(shader, lights[i]);

			BeginDrawing();
		});
		ecs.system("graphics::clear").kind(flecs::OnUpdate).run([](flecs::iter& it) {
			ClearBackground(RAYWHITE);
		});
		ecs.system("graphics::begin_mode_3d").kind(flecs::OnUpdate).run([](flecs::iter& it) {
			BeginMode3D(graphics::camera);
			DrawGrid(12, 10.0f / 12);

			// for(int i = 0; i < MAX_LIGHTS; i++)
			// {
			// 	Color lightColor = Color{static_cast<unsigned char>(lights[i].color[0] * 255),
			// 							 static_cast<unsigned char>(lights[i].color[1] * 255),
			// 							 static_cast<unsigned char>(lights[i].color[2] * 255),
			// 							 static_cast<unsigned char>(lights[i].color[3] * 255)};

			// 	if(lights[i].enabled)
			// 		DrawSphereEx(lights[i].position, 0.2f, 8, 8, lightColor);
			// 	else
			// 		DrawSphereWires(lights[i].position, 0.2f, 8, 8, ColorAlpha(lightColor, 0.3f));
			// }
		});

		// ecs.system("graphics::draw_particle_3d").kind(flecs::OnStore).run([this](flecs::iter& it) {
		// 	// query particles and draw
		// });

		ecs.system("graphics::end_mode_3d").kind(flecs::OnStore).run([](flecs::iter& it) {
			EndMode3D();

			DrawFPS(20, 20);
		});
		ecs.system("graphics::end").kind(flecs::OnStore).run([](flecs::iter& it) { EndDrawing(); });
	}

	void graphics::init_window(flecs::world& world, int width, int height, const char* title)
	{
		/*
		FLAG_VSYNC_HINT
		FLAG_FULLSCREEN_MODE    -> not working properly -> wrong scaling!
		FLAG_WINDOW_RESIZABLE
		FLAG_WINDOW_UNDECORATED
		FLAG_WINDOW_TRANSPARENT
		FLAG_WINDOW_HIDDEN
		FLAG_WINDOW_MINIMIZED   -> Not supported on window creation
		FLAG_WINDOW_MAXIMIZED   -> Not supported on window creation
		FLAG_WINDOW_UNFOCUSED
		FLAG_WINDOW_TOPMOST
		FLAG_WINDOW_HIGHDPI     -> errors after minimize-resize, fb size is recalculated
		FLAG_WINDOW_ALWAYS_RUN
		FLAG_MSAA_4X_HINT
		*/
		SetWindowState(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT | FLAG_WINDOW_HIGHDPI);

		InitWindow(width, height, title);
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
		// flecs owns the main loop
		if(s_world)
		{
			// calling `progress` will invoke registered systems including
			// render pipeline. (clear->draw->swapchain)
			s_world->progress();
		}

		// run post hook here
		if(s_update_func)
		{
			s_update_func();
		}
	}

	void graphics::main_loop_rAF()
	{
		if(s_world)
		{
			s_world->progress();
		}

		if(s_update_func)
		{
			s_update_func();
		}
	}

	void graphics::run_main_loop(flecs::world& world, std::function<void()> update_func)
	{
		s_world = &world;
		s_update_func = update_func;

#if defined(__EMSCRIPTEN__)
		std::cout << "graphics::invoking emscripten rAF loop" << std::endl;
		emscripten_set_main_loop(main_loop_rAF, 0, 1);
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

	// Create light with provided data
	// NOTE: It updated the global lightCount and it's limited to MAX_LIGHTS
	static Light CreateLight(
		int type, Vector3 position, Vector3 target, Color color, float intensity, Shader shader)
	{
		Light light = {0};

		if(lightCount < MAX_LIGHTS)
		{
			light.enabled = 1;
			light.type = type;
			light.position = position;
			light.target = target;
			light.color[0] = (float)color.r / 255.0f;
			light.color[1] = (float)color.g / 255.0f;
			light.color[2] = (float)color.b / 255.0f;
			light.color[3] = (float)color.a / 255.0f;
			light.intensity = intensity;

			// NOTE: Shader parameters names for lights must match the requested ones
			light.enabledLoc =
				GetShaderLocation(shader, TextFormat("lights[%i].enabled", lightCount));
			light.typeLoc = GetShaderLocation(shader, TextFormat("lights[%i].type", lightCount));
			light.positionLoc =
				GetShaderLocation(shader, TextFormat("lights[%i].position", lightCount));
			light.targetLoc =
				GetShaderLocation(shader, TextFormat("lights[%i].target", lightCount));
			light.colorLoc = GetShaderLocation(shader, TextFormat("lights[%i].color", lightCount));
			light.intensityLoc =
				GetShaderLocation(shader, TextFormat("lights[%i].intensity", lightCount));

			UpdateLight(shader, light);

			lightCount++;
		}

		return light;
	}

	// Send light properties to shader
	// NOTE: Light shader locations should be available
	static void UpdateLight(Shader shader, Light light)
	{
		SetShaderValue(shader, light.enabledLoc, &light.enabled, SHADER_UNIFORM_INT);
		SetShaderValue(shader, light.typeLoc, &light.type, SHADER_UNIFORM_INT);

		// Send to shader light position values
		float position[3] = {light.position.x, light.position.y, light.position.z};
		SetShaderValue(shader, light.positionLoc, position, SHADER_UNIFORM_VEC3);

		// Send to shader light target position values
		float target[3] = {light.target.x, light.target.y, light.target.z};
		SetShaderValue(shader, light.targetLoc, target, SHADER_UNIFORM_VEC3);
		SetShaderValue(shader, light.colorLoc, light.color, SHADER_UNIFORM_VEC4);
		SetShaderValue(shader, light.intensityLoc, &light.intensity, SHADER_UNIFORM_FLOAT);
	}

} // namespace graphics