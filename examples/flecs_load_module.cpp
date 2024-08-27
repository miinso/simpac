#include "flecs.h"
#include "graphics_module.h"
#include "model_module.h"
#include <Eigen/Dense>
#include <iostream>

struct Scene {
	Eigen::Vector3f gravity{0, 0, 0};
	int iteration_count = 1;
	int num_substeps = 1;
	float dt;
};

struct Particle {
	float M;
	float Minv;
	Eigen::Vector3f position;
	Eigen::Vector3f velocity;
	Eigen::Vector3f prev_position;
};

struct DistanceConstraint {
	flecs::entity a;
	flecs::entity b;
	float rest_length;
	float lambda = 0;
	float stiffness = 1e+6;
};

void integrate_particle(flecs::iter& it, size_t, Particle& particle, const Scene& scene)
{
	const auto g = scene.gravity;
	const auto dt = scene.dt;

	if(particle.Minv != 0)
	{
		particle.velocity += dt * g;
		particle.position += dt * particle.velocity;
	}
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
		// Constraint solving logic here
	}
}

int main(int, char*[])
{
	flecs::world ecs;

	// TODO: maintain `window` module and `graphics` module separately
	ecs.import <graphics::graphics>();
	graphics::init_graphics(ecs, 800, 600, "Particle Simulation", 60);

	ecs.import <model::model>();

	ecs.component<Scene>();
	ecs.component<Particle>();
	ecs.component<DistanceConstraint>();

	Scene& scene = ecs.ensure<Scene>();
	scene.gravity = Eigen::Vector3f(0, 0, 0);
	scene.iteration_count = 1;
	scene.num_substeps = 1;
	scene.dt = 1.0f / 30.0f;

	ecs.set_target_fps(60);

	// def add_particle(
	// 	pos: Vec3,
	// 	vel: Vec3,
	// 	mass: float,
	// 	radius: float = None,
	// 	flags: uint32 = PARTICLE_FLAG_ACTIVE
	// )
	// int pid1 = model.add_particle();
	// int pid2 = model.add_particle();
	// model.add_spring(pid1, pid2, stiffness)

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

	// Add particle rendering system here
	ecs.system<const Particle, const graphics::RaylibRenderer>()
		.term_at(1)
		.singleton()
		.kind(flecs::OnUpdate)
		.each([](flecs::iter& it,
				 size_t index,
				 const Particle& particle,
				 const graphics::RaylibRenderer& renderer) {
			DrawSphere(Vector3{particle.position.x(), particle.position.y(), particle.position.z()},
					   renderer.particleRadius,
					   renderer.particleColor);
		});

	while(!WindowShouldClose())
	{
		ecs.progress();
	}

	CloseWindow();

	return 0;
}