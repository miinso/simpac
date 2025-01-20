#include "Eigen/Dense"
#include "flecs.h"
#include "graphics_module.h"
#include <array>
#include <iostream>
#include <random>

// Constants
constexpr int CANVAS_WIDTH = 800;
constexpr int CANVAS_HEIGHT = 600;
constexpr float EPSILON = 1e-6f;
constexpr float COLLISION_THRESHOLD = 0.01f;
constexpr float RESTING_VELOCITY_THRESHOLD = 0.01f;
constexpr float RESTING_DISTANCE_THRESHOLD = 0.01f;
constexpr float RESTING_FORCE_THRESHOLD = 0.01f;

// Component definitions
struct Position {
	Eigen::Vector3f value;
};

struct Velocity {
	Eigen::Vector3f value;
};

struct ParticleProperties {
	float radius;
	float mass;
	float friction;
	float restitution;
	bool is_rest;
};

struct PlaneProperties {
	Eigen::Vector3f normal;
	float friction;
	float restitution;
	int collision_count;
};

// Resource (singleton) definitions
struct Scene {
	Eigen::Vector3f gravity;
	float air_resistance;
	float drawHZ;
	float timestep;
	float current_time;
};

struct SimulationResources {
	Model sphere_model;
	Model plane_model;
	Model cube_model;
	Texture2D texture;
};

// Tag components
struct Particle { };
struct Plane { };

// Utility functions
Vector3 e2r(const Eigen::Vector3f& v)
{
	return {v.x(), v.y(), v.z()};
}

Eigen::Vector3f r2e(const Vector3& v)
{
	return Eigen::Vector3f(v.x, v.y, v.z);
}

void simulate_particles(
	flecs::iter& it, size_t index, Position& p, Velocity& v, const ParticleProperties& props)
{
	auto scene = it.world().get<Scene>();
	auto dt = scene->timestep;

	if(!props.is_rest)
	{
		v.value += dt * (1.0f / props.mass) * scene->gravity;
		p.value += dt * v.value;
	}
}

void handle_particle_plane_collisions(
	flecs::iter& it, size_t index, Position& p, Velocity& v, ParticleProperties& props)
{
	auto scene = it.world().get<Scene>();

	it.world().each(
		[&](flecs::entity e, const Position& plane_pos, const PlaneProperties& plane_props) {
			float penetration = (p.value - plane_pos.value).dot(plane_props.normal) - props.radius;

			if(penetration < 0)
			{
				// Collision response
				Eigen::Vector3f v_n = v.value.dot(plane_props.normal) * plane_props.normal;
				Eigen::Vector3f v_t = v.value - v_n;

				v_n = -props.restitution * v_n;

				float friction_magnitude = std::min(plane_props.friction * v_n.norm(), v_t.norm());
				v_t -= friction_magnitude * v_t.normalized();

				v.value = v_n + v_t;

				// Adjust position
				p.value -= (penetration - EPSILON) * plane_props.normal;

				// Update collision count
				auto mutable_plane = e.get_mut<PlaneProperties>();
				mutable_plane->collision_count++;
			}
		});
}

void check_particle_rest_state(
	flecs::iter& it, size_t index, Position& p, Velocity& v, ParticleProperties& props)
{
	auto scene = it.world().get<Scene>();

	if(v.value.norm() < RESTING_VELOCITY_THRESHOLD)
	{
		bool at_rest = false;
		it.world().each([&](const Position& plane_pos, const PlaneProperties& plane_props) {
			float d = (p.value - plane_pos.value).dot(plane_props.normal) - props.radius;
			if(std::abs(d) < RESTING_DISTANCE_THRESHOLD)
			{
				Eigen::Vector3f total_force = scene->gravity * props.mass;
				float normal_force = total_force.dot(plane_props.normal);
				if(normal_force < -RESTING_FORCE_THRESHOLD)
				{
					Eigen::Vector3f tangential_force =
						total_force - normal_force * plane_props.normal;
					if(tangential_force.norm() <= plane_props.friction * std::abs(normal_force))
					{
						at_rest = true;
					}
				}
			}
		});
		props.is_rest = at_rest;
	}
	else
	{
		props.is_rest = false;
	}
}

void draw_particles(flecs::iter& it,
					size_t index,
					const Position& p,
					const ParticleProperties& props)
{
	auto res = it.world().get<SimulationResources>();
	DrawModel(res->sphere_model, e2r(p.value), props.radius, props.is_rest ? GRAY : WHITE);
}

void draw_planes(flecs::iter& it, size_t index, const Position& p, const PlaneProperties& props)
{
	auto res = it.world().get<SimulationResources>();
	const std::array<Color, 4> PLANE_COLORS = {Color{66, 133, 244, 255},
											   Color{234, 67, 53, 255},
											   Color{251, 188, 5, 255},
											   Color{52, 168, 83, 255}};

	Eigen::Vector3f position = p.value;
	Eigen::Vector3f normal = props.normal.normalized();

	Eigen::Vector3f up = Eigen::Vector3f::UnitY();
	if(normal.dot(up) > 0.99f || normal.dot(up) < -0.99f)
	{
		up = Eigen::Vector3f::UnitZ();
	}
	Eigen::Vector3f right = up.cross(normal).normalized();
	up = normal.cross(right).normalized();

	Color color =
		props.collision_count ? PLANE_COLORS[props.collision_count % PLANE_COLORS.size()] : WHITE;

	DrawModelEx(res->cube_model,
				e2r(position),
				e2r(right),
				std::acos(Eigen::Vector3f::UnitY().dot(normal)) * RAD2DEG,
				e2r(Eigen::Vector3f::Ones() * 5),
				color);

	DrawLine3D(e2r(position), e2r(position + normal), RED);
	DrawSphere(e2r(position + normal), 0.1f, RED);
}

void handle_input(flecs::iter& it, size_t index, Position& p, Velocity& v)
{
	const auto& camera = graphics::graphics::camera;

	if(IsKeyDown(KEY_ONE))
	{
		p.value = Eigen::Vector3f(0, 5, 0);
		v.value = Eigen::Vector3f(9, 0, 0);
	}

	if(IsKeyDown(KEY_TWO))
	{
		p.value = Eigen::Vector3f(0, 5, 0);
		v.value = Eigen::Vector3f(9, 0, 9);
	}
}

void reset_particles(flecs::iter& it)
{
	if(IsKeyPressed(KEY_R))
	{
		it.world().each([&](flecs::entity e, const Particle&) { e.destruct(); });
	}
}

void create_particle_on_input(flecs::iter& it)
{
	const auto& camera = graphics::graphics::camera;

	if(IsKeyPressed(KEY_SPACE))
	{
		const auto origin = r2e(camera.position);
		const auto target = r2e(camera.target);
		const auto velocity =
			(target - origin + Eigen::Vector3f::UnitY() * 2.0f).normalized() * 10.0f;

		it.world()
			.entity()
			.add<Particle>()
			.set<Position>({origin})
			.set<Velocity>({velocity})
			.set<ParticleProperties>({0.5f, 1.0f, 0.3f, 0.7f, false});
	}
}

void handle_particle_particle_collisions(
	flecs::iter& it, size_t index, Position& p, Velocity& v, ParticleProperties& props)
{
	auto world = it.world();

	world.each([&](flecs::entity other_entity,
				   const Position& other_p,
				   const ParticleProperties& other_props) {
		if(it.entity(index) == other_entity)
			return;

		Eigen::Vector3f delta = p.value - other_p.value;
		float distance = delta.norm();
		float min_distance = props.radius + other_props.radius;

		if(distance < min_distance)
		{
			Eigen::Vector3f normal = delta.normalized();

			auto other_v = other_entity.get<Velocity>();
			Eigen::Vector3f rel_velocity = v.value - other_v->value;

			float impulse = -(1 + props.restitution) * rel_velocity.dot(normal) /
							((1 / props.mass) + (1 / other_props.mass));

			v.value += impulse / props.mass * normal;
			Eigen::Vector3f new_other_v = other_v->value - impulse / other_props.mass * normal;

			float overlap = min_distance - distance;
			Eigen::Vector3f separation = normal * overlap * 0.5f;
			p.value += separation;
			Eigen::Vector3f new_other_p = other_p.value - separation;

			other_entity.set<Velocity>({new_other_v});
			other_entity.set<Position>({new_other_p});
		}
	});
}

int main()
{
	std::cout << "Hi from " << __FILE__ << std::endl;

	flecs::world ecs;

	// Register components
	ecs.component<Position>();
	ecs.component<Velocity>();
	ecs.component<ParticleProperties>();
	ecs.component<PlaneProperties>();
	ecs.component<Particle>();
	ecs.component<Plane>();

	// Set up resources
	ecs.set<Scene>({
		-9.8f * Eigen::Vector3f::UnitY(), // gravity
		0.0f, // air_resistance
		60.0f, // drawHZ
		1.0f / 60.0f, // timestep
		0.0f // current_time
	});

	// Import and initialize graphics
	ecs.import <graphics::graphics>();
	graphics::graphics::init_window(ecs, CANVAS_WIDTH, CANVAS_HEIGHT, "Flecs Particle Simulator");
	// ecs.progress();

	// Initialize simulation resources, must be called after graphics::init
	SimulationResources res;
	Image checked = GenImageChecked(6, 6, 1, 1, LIGHTGRAY, DARKGRAY);
	res.texture = LoadTextureFromImage(checked);
	UnloadImage(checked);

	res.plane_model = LoadModelFromMesh(GenMeshPlane(2, 2, 4, 3));
	res.plane_model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = res.texture;
	// res.plane_model.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = RED;

	Mesh cubeMesh = GenMeshCube(2, 0.0f, 2);
	GenMeshTangents(&cubeMesh);
	res.cube_model = LoadModelFromMesh(cubeMesh);
	res.cube_model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = res.texture;
	// res.cube_model.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = GREEN;

	res.sphere_model = LoadModelFromMesh(GenMeshSphere(1, 64, 64));
	res.sphere_model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = res.texture;
	ecs.set<SimulationResources>(res);

	// Create entities
	auto add_particle = [&](const Eigen::Vector3f& pos, const Eigen::Vector3f& vel) {
		ecs.entity()
			.add<Particle>()
			.set<Position>({pos})
			.set<Velocity>({vel})
			.set<ParticleProperties>({0.5f, 1.0f, 0.3f, 0.7f, false});
	};

	auto add_plane = [&](const Eigen::Vector3f& pos, const Eigen::Vector3f& normal) {
		ecs.entity().add<Plane>().set<Position>({pos}).set<PlaneProperties>(
			{normal, 0.3f, 0.7f, 0});
	};

	add_particle({1, 5, 0}, {5, -10, 10});
	add_plane({5, 5, 0}, {-1, 0, 0});
	add_plane({-5, 5, 0}, {1, 0, 0});
	add_plane({0, -0.01, 0}, {0, 1, 0});
	add_plane({0, 5, 5}, {0, 0, -1});
	add_plane({0, 5, -5}, {0, 0, 1});

	// Set up systems
	ecs.system<Position, Velocity, const ParticleProperties>("SimulateParticles")
		.kind(flecs::OnUpdate)
		.each(simulate_particles);

	ecs.system<Position, Velocity, ParticleProperties>("HandleCollisions")
		.kind(flecs::OnUpdate)
		.each(handle_particle_plane_collisions);

	ecs.system<Position, Velocity, ParticleProperties>("CheckParticleRestState")
		.kind(flecs::OnUpdate)
		.each(check_particle_rest_state);

	ecs.system<const Position, const ParticleProperties>("DrawParticles")
		.kind(flecs::OnUpdate)
		.each(draw_particles);

	ecs.system<const Position, const PlaneProperties>("DrawPlanes")
		.kind(flecs::OnUpdate)
		.each(draw_planes);

	ecs.system<Position, Velocity>("HandleInput").kind(flecs::PreUpdate).each(handle_input);

	ecs.system("CreateParticleOnInput").kind(flecs::PreUpdate).run(create_particle_on_input);

	ecs.system<Position, Velocity, ParticleProperties>("HandleParticleCollisions")
		.kind(flecs::OnUpdate)
		.each(handle_particle_particle_collisions);

	ecs.system("ResetParticles").kind(flecs::PreUpdate).run(reset_particles);

	// Run the main loop
	graphics::graphics::run_main_loop(ecs, []() {
		// Additional main loop callback if needed
	});

	std::cout << "Simulation ended." << std::endl;
	return 0;
}