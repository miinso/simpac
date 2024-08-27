#include "flecs.h"
#include "raylib.h"

#include <Eigen/Dense>
#include <iostream>

// void SpawnEnemy(flecs::iter& it, size_t, const Game& g) {
//     const Level* lvl = g.level.get<Level>();

//     it.world().entity().child_of<enemies>().is_a<prefabs::Enemy>()
//         .set<Direction>({0})
//         .set<Position>({
//             lvl->spawn_point.x, 1.2, lvl->spawn_point.y
//         });
// }

// using namespace Eigen;

struct Scene {
	flecs::entity window;

	// Position center;
	Eigen::Vector3f gravity{0, 0, 0};
	int iteration_count = 1;
	int num_substeps = 1;
	float dt;
	// float size;
};

struct Particle {
	float M;
	float Minv;
	Eigen::Vector3f position;
	Eigen::Vector3f velocity;
	Eigen::Vector3f prev_position;
};

struct Rigidbody {
	Eigen::Vector3f x;
};

struct DistanceConstraint {
	// Particle a;
	flecs::entity a;
	flecs::entity b;
	float rest_length;
	float lambda = 0;
	float stiffness = 1e+6;
};

// void integrate_particle(flecs::iter& it, Particle* p, const Scene* scene) {
//     for (auto i : it) {
//         if (p[i].Minv != 0) {
//             p[i].velocity += scene->dt * scene->gravity;
//             p[i].position += scene->dt * p[i].velocity;
//         }
//     }
// }

void integrate_particle(flecs::iter& it, size_t, Particle& particle, const Scene& scene)
{
	// Scene& g = ecs.ensure<Game>();
	const auto g = scene.gravity;
	const auto dt = scene.dt;

	if(particle.Minv != 0)
	{
		particle.velocity += dt * g;
		particle.position += dt * particle.velocity;
	}
	// Eigen::IOFormat CleanFmt(3, 0, ", ", "\n", "[", "]");
	// std::cout << "pos: " << particle.position.transpose().format(CleanFmt)
	// 		  << " vel: " << particle.velocity.transpose().format(CleanFmt) << std::endl;
}

void solve_distance_constraint(flecs::iter& it,
							   size_t,
							   DistanceConstraint& constraint,
							   const Scene& scene)
{
	const auto dt = scene.dt;

	const auto* pa = constraint.a.get<Particle>();
	const auto* pb = constraint.b.get<Particle>();

	if(pa && pb)
	{
		// std::cout << "solve" << std::endl;
		// Eigen::IOFormat CleanFmt(3, 0, ", ", "\n", "[", "]");
		// std::cout << "pos1: " << pa->position.transpose().format(CleanFmt)
		// 		  << " vel1: " << pa->velocity.transpose().format(CleanFmt) << std::endl;
		// std::cout << "pos2: " << pb->position.transpose().format(CleanFmt)
		// 		  << " vel2: " << pb->velocity.transpose().format(CleanFmt) << std::endl;
	}
}

////
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

class RaylibModule
{
public:
	RaylibModule(flecs::world& world)
	{
		world.module<RaylibModule>();

		world.component<RaylibWindow>();
		world.component<RaylibCamera>();
		world.component<RaylibRenderer>();

		world.system<RaylibWindow>()
			.kind(flecs::OnStart)
			.each([](flecs::entity e, RaylibWindow& window) {
				InitWindow(window.width, window.height, window.title);
				SetTargetFPS(window.targetFPS);
			});

		world.system<RaylibWindow>()
			.kind(flecs::PostFrame)
			.each([](flecs::entity e, RaylibWindow& window) {
				EndMode3D();
				EndDrawing();
			});

		world.system<RaylibCamera>()
			.kind(flecs::OnStart)
			.each([](flecs::entity e, RaylibCamera& cam) {
				cam.camera.position = Vector3{5.0f, 5.0f, 5.0f};
				cam.camera.target = Vector3{0.0f, 0.0f, 0.0f};
				cam.camera.up = Vector3{0.0f, 1.0f, 0.0f};
				cam.camera.fovy = 60.0f;
				cam.camera.projection = CAMERA_PERSPECTIVE;
			});

		world.system<RaylibCamera>()
			.kind(flecs::PreUpdate)
			.each([](flecs::entity e, RaylibCamera& cam) {
				if(IsMouseButtonDown(MOUSE_LEFT_BUTTON))
				{
					UpdateCamera(&cam.camera, CAMERA_THIRD_PERSON);
				}
			});

		// world.system<const RaylibCamera>()
		// 	.kind(flecs::PreUpdate)
		// 	.each([](flecs::entity e, const RaylibCamera& cam) { BeginMode3D(cam.camera); });

		// world.system<>().kind(flecs::PostUpdate).each([](flecs::entity e) { EndMode3D(); });

		world.system<const RaylibRenderer, const RaylibCamera>()
			.kind(flecs::PreUpdate)
			.each([](flecs::entity e, const RaylibRenderer& renderer, const RaylibCamera& camera) {
				BeginDrawing();
				ClearBackground(renderer.backgroundColor);
				DrawFPS(10, 10);

				BeginMode3D(camera.camera);

				DrawSphere(Vector3{2, 0, 0}, 0.1, GREEN);
			});

		world.system<const Particle, const RaylibRenderer>()
			.term_at(1)
			.singleton()
			.kind(flecs::OnUpdate)
			.each([](flecs::iter& it,
					 size_t index,
					 const Particle& particle,
					 const RaylibRenderer& renderer) {
				// const auto* renderer = it.world().get<RaylibRenderer>();
				// if(renderer)
				// {
				DrawSphere(
					Vector3{particle.position.x(), particle.position.y(), particle.position.z()},
					renderer.particleRadius,
					renderer.particleColor);

				// }
			});

		// world
		// 	.system<const DistanceConstraint,
		// 			const Particle,
		// 			const Particle,
		// 			const RaylibRenderer>()
		// 	.kind(flecs::OnStore)
		// 	.iter([](flecs::iter& it,
		// 			 const DistanceConstraint* c,
		// 			 const Particle* p1,
		// 			 const Particle* p2,
		// 			 const RaylibRenderer* renderer) {
		// 		for(auto i : it)
		// 		{
		// 			Vector3f pos1 = p1[i].position;
		// 			Vector3f pos2 = p2[i].position;
		// 			DrawLine(pos1.x(), pos1.y(), pos2.x(), pos2.y(), renderer->constraintColor);
		// 		}
		// 	});
	}
};
////

int main(int, char*[])
{
	flecs::world ecs;

	ecs.import <RaylibModule>();

	// 윈도우 및 렌더러 설정
	ecs.entity()
		.set<RaylibWindow>({800, 600, "Particle Simulation", 60})
		.set<RaylibRenderer>({
			BLUE, // backgroundColor
			5.0f, // particleRadius
			RED, // particleColor
			BLUE // constraintColor
		})
		.set<RaylibCamera>({});
	ecs.progress();

	ecs.component<Scene>();
	ecs.component<Particle>();
	ecs.component<DistanceConstraint>();

	// auto scene = ecs.entity();
	// Scene scene_;
	// scene_.gravity = Vector3f(0, -9.81, 0);
	// scene_.iteration_count = 1;
	// scene_.num_substeps = 1;
	// scene_.dt = 1.0f / 30.0f;
	// scene.set<Scene>(scene_);

	Scene& scene = ecs.ensure<Scene>();
	scene.gravity = Eigen::Vector3f(0, 0, 0);
	scene.iteration_count = 1;
	scene.num_substeps = 1;
	scene.dt = 1.0f / 30.0f;

	ecs.set_target_fps(60);

	// add_particle
	// add_body
	auto particle1 = ecs.entity().set<Particle>(
		{1.0f, 1.0f, Eigen::Vector3f(0, 1, 0), Eigen::Vector3f::Zero(), Eigen::Vector3f(0, 1, 0)});

	auto particle2 = ecs.entity().set<Particle>(
		{1.0f, 1.0f, Eigen::Vector3f(1, 1, 0), Eigen::Vector3f::Zero(), Eigen::Vector3f(1, 1, 0)});

	ecs.entity().set<DistanceConstraint>({particle1, particle2, 1.0f, 0.0f, 1e6f});

	ecs.system<Particle, const Scene>("IntegrateParticles")
		.term_at(1)
		.singleton()
		.kind(flecs::OnUpdate)
		.each(integrate_particle);

	ecs.system<DistanceConstraint, const Scene>("SolveDistanceConstraints")
		.term_at(1)
		.singleton()
		.kind(flecs::OnUpdate)
		.each(solve_distance_constraint);

	ecs.system<RaylibCamera>().each([](flecs::entity e, RaylibCamera& cam) {
		// if(IsMouseButtonDown(MOUSE_LEFT_BUTTON))
		// {
		// 	UpdateCamera(&cam.camera, CAMERA_THIRD_PERSON);
		// }
		std::cout << cam.camera.position.x << std::endl;
	});
	// ecs.system().run(
	// 	[](flecs::iter& it) { std::cout << "delta_time: " << it.delta_time() << std::endl; });

	// init window
	// InitWindow(1280, 720, "Rigid Body Tutorial");
	// SetTargetFPS(60);
	while(!WindowShouldClose())
	{
		ecs.progress();
	}

	CloseWindow();

	// run
	// for(int i = 0; i < 10; ++i)
	// {
	// 	ecs.progress();
	// }

	// std::cout << "wow" << std::endl;
	return 0;
}