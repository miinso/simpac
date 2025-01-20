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

// Forward declarations
struct Scene;
struct Particle;
struct Plane;
struct Contact;

// Component definitions
struct Scene {
	Eigen::Vector3f gravity;
	float air_resistance;
	float drawHZ;
	float timestep;
	float current_time;
};

struct Particle {
	Eigen::Vector3f position;
	Eigen::Vector3f velocity;
	Eigen::Vector3f position_old;
	Eigen::Vector3f velocity_old;
	float radius;
	float mass;
	float friction;
	float restitution;
	bool is_rest;
};

struct Plane {
	Eigen::Vector3f position;
	Eigen::Vector3f normal;
	float friction;
	float restitution;
	float mass;
	int collision_count;

	Plane()
		: position(Eigen::Vector3f::Zero())
		, normal(Eigen::Vector3f::UnitY())
		, friction(0.3f)
		, restitution(0.7f)
		, collision_count(0)
	{ }
};

struct Contact {
	float time_of_impact;
	Eigen::Vector3f normal;
	flecs::entity plane;
};

// Global variables
flecs::world ecs;
Model sphere_model;
Model plane_model;
Model globalCubeModel;
Texture2D texture0;

const std::array<Color, 4> PLANE_COLORS = {Color{66, 133, 244, 255},
										   Color{234, 67, 53, 255},
										   Color{251, 188, 5, 255},
										   Color{52, 168, 83, 255}};

// Function declarations
void init_models();
void add_plane(const Eigen::Vector3f& position, const Eigen::Vector3f& normal);
flecs::entity add_particle(const Eigen::Vector3f& position,
						   const Eigen::Vector3f& velocity = Eigen::Vector3f::Zero());
Vector3 eigen_to_raylib(const Eigen::Vector3f& v);
float calculate_penetration_depth(const Particle& p, const Plane& plane);
bool detect_collision(const Particle& p_old, const Particle& p_new, const Plane& plane, float& f);
std::vector<Contact> detect_collisions(const Particle& p_old,
									   const Particle& p_new,
									   const std::vector<flecs::entity>& planes);
Particle handle_multiple_collisions(Particle p, const std::vector<Contact>& contacts);
Particle integrate_particle(const Particle& p, const Scene& scene, float dt);
Particle collision_response(Particle p, const Plane& plane);
bool is_particle_at_rest(const Particle& p,
						 const Scene& scene,
						 const std::vector<flecs::entity>& planes);
void simulate_particle(flecs::iter& it, size_t, Particle& p, const Scene& scene);
void draw_plane(flecs::iter& it, size_t index, Plane& p);
void handle_input(Particle& p);

int main()
{
	std::cout << "Starting simulation from " << __FILE__ << std::endl;

	Scene scene{-9.8f * Eigen::Vector3f::UnitY(), 0.0f, 60.0f, 1.0f / 60.0f, 0.0f};
	ecs.set<Scene>(scene);

	ecs.import <graphics::graphics>();
	graphics::graphics::init_window(ecs, CANVAS_WIDTH, CANVAS_HEIGHT, "Particle Simulator");

	init_models();

	add_particle({1, 5, 0}, {5, -10, 10});
	add_plane({5, 5, 0}, {-1, 0, 0});
	add_plane({-5, 5, 0}, {1, 0, 0});
	add_plane({0, -0.01, 0}, {0, 1, 0});
	add_plane({0, 5, 5}, {0, 0, -1});
	add_plane({0, 5, -5}, {0, 0, 1});

	ecs.system<Particle, const Scene>("Simulator")
		.term_at(1)
		.singleton()
		.kind(flecs::OnUpdate)
		.each(simulate_particle);

	ecs.system<Plane>("DrawPlane").kind(flecs::OnUpdate).each(draw_plane);

	ecs.system<Particle>("HandleInput")
		.kind(flecs::PreUpdate)
		.each([](flecs::entity e, Particle& p) { handle_input(p); });

	graphics::graphics::run_main_loop(ecs, []() {
		// Additional main loop callback if needed
	});

	std::cout << "Simulation ended." << std::endl;
	return 0;
}

// Function implementations
void init_models()
{
	Image checked = GenImageChecked(6, 6, 1, 1, LIGHTGRAY, DARKGRAY);
	texture0 = LoadTextureFromImage(checked);
	UnloadImage(checked);

	plane_model = LoadModelFromMesh(GenMeshPlane(2, 2, 4, 3));
	plane_model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture0;

	Mesh cubeMesh = GenMeshCube(2, 0.0f, 2);
	GenMeshTangents(&cubeMesh);
	globalCubeModel = LoadModelFromMesh(cubeMesh);
	globalCubeModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture0;

	sphere_model = LoadModelFromMesh(GenMeshSphere(1, 64, 64));
	sphere_model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture0;
}

void add_plane(const Eigen::Vector3f& position, const Eigen::Vector3f& normal)
{
	Plane plane;
	plane.position = position;
	plane.normal = normal;
	ecs.entity().set<Plane>(plane);
}

flecs::entity add_particle(const Eigen::Vector3f& position, const Eigen::Vector3f& velocity)
{
	Particle p{position, velocity, position, velocity, 0.5f, 1.0f, 0.3f, 0.7f, false};
	return ecs.entity().set<Particle>(p);
}

Vector3 eigen_to_raylib(const Eigen::Vector3f& v)
{
	return {v.x(), v.y(), v.z()};
}

float calculate_penetration_depth(const Particle& p, const Plane& plane)
{
	return (p.position - plane.position).dot(plane.normal) - p.radius;
}

bool detect_collision(const Particle& p_old, const Particle& p_new, const Plane& plane, float& f)
{
	float d_old = calculate_penetration_depth(p_old, plane);
	float d_new = calculate_penetration_depth(p_new, plane);

	if(d_old * d_new < 0 || std::abs(d_new) < EPSILON)
	{
		f = d_old / (d_old - d_new);
		f = std::min(std::max(f, 0.0f), 1.0f);
		return true;
	}
	return false;
}

std::vector<Contact> detect_collisions(const Particle& p_old,
									   const Particle& p_new,
									   const std::vector<flecs::entity>& planes)
{
	std::vector<Contact> contacts;

	for(const auto& plane_entity : planes)
	{
		const Plane* plane = plane_entity.get<Plane>();
		float f;
		if(detect_collision(p_old, p_new, *plane, f))
		{
			contacts.push_back({f, plane->normal, plane_entity});
			Plane* mutable_plane = plane_entity.get_mut<Plane>();
			mutable_plane->collision_count++;
		}
	}

	std::sort(contacts.begin(), contacts.end(), [](const Contact& a, const Contact& b) {
		return a.time_of_impact < b.time_of_impact;
	});

	std::vector<Contact> simultaneous_contacts;
	if(!contacts.empty())
	{
		simultaneous_contacts.push_back(contacts[0]);
		for(size_t i = 1; i < contacts.size(); ++i)
		{
			if(contacts[i].time_of_impact - contacts[0].time_of_impact < COLLISION_THRESHOLD)
			{
				simultaneous_contacts.push_back(contacts[i]);
			}
			else
			{
				break;
			}
		}
	}

	return simultaneous_contacts;
}

Particle handle_multiple_collisions(Particle p, const std::vector<Contact>& contacts)
{
	Eigen::Vector3f total_normal = Eigen::Vector3f::Zero();
	for(const auto& contact : contacts)
	{
		total_normal += contact.normal;
	}
	total_normal.normalize();

	Eigen::Vector3f v_n = p.velocity.dot(total_normal) * total_normal;
	Eigen::Vector3f v_t = p.velocity - v_n;

	v_n = -p.restitution * v_n;

	float avg_friction = 0.0f;
	for(const auto& contact : contacts)
	{
		const Plane* plane = contact.plane.get<Plane>();
		avg_friction += plane->friction;
	}
	avg_friction /= contacts.size();

	float friction_magnitude = std::min(avg_friction * v_n.norm(), v_t.norm());
	v_t -= friction_magnitude * v_t.normalized();

	p.velocity = v_n + v_t;

	for(const auto& contact : contacts)
	{
		const Plane* plane = contact.plane.get<Plane>();
		float penetration = calculate_penetration_depth(p, *plane);
		if(penetration < 0)
		{
			p.position -= (penetration - EPSILON) * plane->normal;
		}
	}

	return p;
}

Particle integrate_particle(const Particle& p, const Scene& scene, float dt)
{
	Particle new_p = p;
	new_p.velocity += dt * (1 / p.mass) * scene.gravity;
	new_p.position += dt * new_p.velocity;
	return new_p;
}

bool is_particle_at_rest(const Particle& p,
						 const Scene& scene,
						 const std::vector<flecs::entity>& planes)
{
	if(p.velocity.norm() >= RESTING_VELOCITY_THRESHOLD)
	{
		return false;
	}

	for(const auto& plane_entity : planes)
	{
		const Plane* plane = plane_entity.get<Plane>();
		float d = calculate_penetration_depth(p, *plane);

		if(std::abs(d) < RESTING_DISTANCE_THRESHOLD)
		{
			Eigen::Vector3f total_force = scene.gravity * p.mass;
			float normal_force = total_force.dot(plane->normal);

			if(normal_force < -RESTING_FORCE_THRESHOLD)
			{
				Eigen::Vector3f tangential_force = total_force - normal_force * plane->normal;

				if(tangential_force.norm() <= plane->friction * std::abs(normal_force))
				{
					return true;
				}
			}
		}
	}

	return false;
}

void simulate_particle(flecs::iter& it, size_t, Particle& p, const Scene& scene)
{
	const float h = scene.timestep;
	float timestep_remaining = h;

	std::vector<flecs::entity> planes;
	it.world().each([&planes](flecs::entity e, Plane&) { planes.push_back(e); });

	if(p.is_rest && !is_particle_at_rest(p, scene, planes))
	{
		p.is_rest = false;
	}

	if(!p.is_rest)
	{
		while(timestep_remaining > EPSILON)
		{
			float timestep = timestep_remaining;
			Particle p_new = integrate_particle(p, scene, timestep);

			std::vector<Contact> contacts = detect_collisions(p, p_new, planes);

			if(!contacts.empty())
			{
				float earliest_time = contacts[0].time_of_impact;
				timestep *= earliest_time;
				p_new = integrate_particle(p, scene, timestep);
				p_new = handle_multiple_collisions(p_new, contacts);
			}

			timestep_remaining -= timestep;
			p = p_new;

			if(timestep < EPSILON)
			{
				break;
			}

			if(is_particle_at_rest(p, scene, planes))
			{
				p.is_rest = true;
				break;
			}
		}
	}

	DrawModel(sphere_model, eigen_to_raylib(p.position), p.radius, p.is_rest ? GRAY : WHITE);

	if(IsKeyDown(KEY_R))
	{
		p.position = {0, 5, 0};
		p.velocity = {9, 0, 9};
	}

	if(IsKeyDown(KEY_SPACE))
	{
		const auto& c = graphics::graphics::camera;
		const auto origin = Eigen::Vector3f(c.position.x, c.position.y, c.position.z);
		const auto target = Eigen::Vector3f(c.target.x, c.target.y, c.target.z);
		const auto aa = origin - target;
		p.position = origin;
		p.velocity = (target - origin + Eigen::Vector3f::UnitY() * 2.0f).normalized() * 10.0f;
	}
}

void draw_plane(flecs::iter& it, size_t index, Plane& p)
{
	Eigen::Vector3f position = p.position;
	Eigen::Vector3f normal = p.normal.normalized();

	Eigen::Vector3f up = Eigen::Vector3f::UnitY();
	if(normal.dot(up) > 0.99f || normal.dot(up) < -0.99f)
	{
		up = Eigen::Vector3f::UnitZ();
	}
	Eigen::Vector3f right = up.cross(normal).normalized();
	up = normal.cross(right).normalized();

	Color color = p.collision_count ? PLANE_COLORS[p.collision_count % PLANE_COLORS.size()] : WHITE;

	DrawModelEx(globalCubeModel,
				eigen_to_raylib(position),
				eigen_to_raylib(right),
				std::acos(Eigen::Vector3f::UnitY().dot(normal)) * RAD2DEG,
				eigen_to_raylib(Eigen::Vector3f::Ones() * 5),
				color);

	DrawLine3D(eigen_to_raylib(position), eigen_to_raylib(position + normal), RED);
	DrawSphere(eigen_to_raylib(position + normal), 0.1f, RED);
}

// New utility functions
Eigen::Vector3f raylib_to_eigen(const Vector3& v)
{
	return Eigen::Vector3f(v.x, v.y, v.z);
}

void handle_input(Particle& p)
{
	if(IsKeyDown(KEY_R))
	{
		p.position = Eigen::Vector3f(0, 5, 0);
		p.velocity = Eigen::Vector3f(9, 0, 9);
	}

	if(IsKeyDown(KEY_SPACE))
	{
		const auto& c = graphics::graphics::camera;
		const auto origin = raylib_to_eigen(c.position);
		const auto target = raylib_to_eigen(c.target);
		p.position = origin;
		p.velocity = (target - origin + Eigen::Vector3f::UnitY() * 2.0f).normalized() * 10.0f;
	}
}