#include "flecs.h"
#include "graphics_module.h"

#include "Eigen/Dense"
#include <iostream>
#include <random>

const int CANVAS_WIDTH = 800;
const int CANVAS_HEIGHT = 600;
const float EPSILON = 1e-6f;
const float COLLISION_THRESHOLD = 0.01f;
const float RESTING_VELOCITY_THRESHOLD = 0.01f;
const float RESTING_DISTANCE_THRESHOLD = 0.01f;
const float RESTING_FORCE_THRESHOLD = 0.01f;

flecs::world ecs;
Model sphere_model; // raylib Model
Model plane_model;
Model globalCubeModel;
Texture2D texture0;

const std::array<Color, 4> PLANE_COLORS = {Color{66, 133, 244, 255},
										   Color{234, 67, 53, 255},
										   Color{251, 188, 5, 255},
										   Color{52, 168, 83, 255}};

struct Scene {
	Eigen::Vector3f gravity;
	float air_resistence;

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
	Plane()
	{
		position = Eigen::Vector3f::Zero();
		normal = Eigen::Vector3f::UnitY();
		friction = 0.3f;
		restitution = 0.7f;
		collision_count = 0;
	}
	Eigen::Vector3f position;
	Eigen::Vector3f normal;

	float friction;
	float restitution;
	float mass;
	int collision_count;
	// d = (x - p).dot(n) + p.radius;
};

struct Collider { 
	// 
	bool enabled;
};

struct Geometry {
	float* vertices;
	int* indices;
	int* faces;
};

void add_plane(Eigen::Vector3f position, Eigen::Vector3f normal)
{
	Plane plane;
	plane.position = position;
	plane.normal = normal;
	ecs.entity().set<Plane>(plane);
}

flecs::entity add_particle(Eigen::Vector3f position,
						   Eigen::Vector3f velocity = Eigen::Vector3f::Zero())
{
	Particle p;
	p.position = position;
	p.velocity = velocity;
	p.radius = 0.5;
	p.mass = 1;
	p.friction = 0.3f;
	p.restitution = 0.7f;

	auto e = ecs.entity();
	e.set<Particle>(p);

	return e;
}

Vector3 e2r(const Eigen::Vector3f& v)
{
	return {v.x(), v.y(), v.z()};
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
				e2r(position),
				e2r(right),
				std::acos(Eigen::Vector3f::UnitY().dot(normal)) * RAD2DEG,
				e2r(Eigen::Vector3f::Ones() * 5),
				color);

	DrawLine3D(e2r(position), e2r(position + normal), RED);
	DrawSphere(e2r(position + normal), 0.1f, RED);

	// auto cubeScreenPosition = GetWorldToScreen(Vector3{position.x(), position.y()- 1.0f, position.z()},
	// 										   graphics::graphics::camera);

	// DrawText("Enemy: 100 / 100",
	// 		 (int)cubeScreenPosition.x - MeasureText("Enemy: 100/100", 20) / 2,
	// 		 (int)cubeScreenPosition.y,
	// 		 20,
	// 		 BLACK);
}

/////

struct Contact {
	float time_of_impact;
	Eigen::Vector3f normal;
	flecs::entity plane;
};

float calculatePenetrationDepth(const Particle& p, const Plane& plane)
{
	return (p.position - plane.position).dot(plane.normal) - p.radius;
}

bool detectCollision(const Particle& p_old, const Particle& p_new, const Plane& plane, float& f)
{
	float d_old = calculatePenetrationDepth(p_old, plane);
	float d_new = calculatePenetrationDepth(p_new, plane);

	if(d_old * d_new < 0 || std::abs(d_new) < EPSILON)
	{
		f = d_old / (d_old - d_new);
		f = std::min(std::max(f, 0.0f), 1.0f); // Clamp f between 0 and 1
		return true;
	}
	return false;
}

std::vector<Contact> detectCollisions(const Particle& p_old,
									  const Particle& p_new,
									  const std::vector<flecs::entity>& planes)
{
	std::vector<Contact> contacts;

	for(const auto& plane_entity : planes)
	{
		const Plane* plane = plane_entity.get<Plane>();
		float f;
		if(detectCollision(p_old, p_new, *plane, f))
		{
			Contact contact;
			contact.time_of_impact = f;
			contact.normal = plane->normal;
			contact.plane = plane_entity;
			contacts.push_back(contact);

			Plane* plane = contact.plane.get_mut<Plane>();
			plane->collision_count++;
			std::cout << "plane " << plane->position.transpose() << ": " << plane->collision_count
					  << std::endl;
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

Particle handleMultipleCollisions(Particle p, const std::vector<Contact>& contacts)
{
	Eigen::Vector3f total_normal = Eigen::Vector3f::Zero();
	for(const auto& contact : contacts)
	{
		total_normal += contact.normal;
	}
	total_normal.normalize();

	Eigen::Vector3f v_n = p.velocity.dot(total_normal) * total_normal;
	Eigen::Vector3f v_t = p.velocity - v_n;

	// Elasticity
	v_n = -p.restitution * v_n;

	// Friction (simplified for multiple contacts)
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

	// Adjust position to prevent penetration
	for(const auto& contact : contacts)
	{
		const Plane* plane = contact.plane.get<Plane>();
		float penetration = calculatePenetrationDepth(p, *plane);
		if(penetration < 0)
		{
			p.position -= (penetration - EPSILON) * plane->normal;
		}
	}

	return p;
}

Particle integrateParticle(const Particle& p, const Scene& scene, float dt)
{
	Particle new_p = p;
	new_p.velocity += dt * (1 / p.mass) * scene.gravity;
	new_p.position += dt * new_p.velocity;
	return new_p;
}

Particle collisionResponse(Particle p, const Plane& plane)
{
	Eigen::Vector3f v_n = p.velocity.dot(plane.normal) * plane.normal;
	Eigen::Vector3f v_t = p.velocity - v_n;

	// Elasticity
	v_n = -p.restitution * v_n;

	// Friction (Coulomb model)
	float friction_magnitude = std::min(plane.friction * v_n.norm(), v_t.norm());
	v_t -= friction_magnitude * v_t.normalized();

	p.velocity = v_n + v_t;

	// Adjust position to prevent penetration
	float penetration = calculatePenetrationDepth(p, plane);
	if(penetration < 0)
	{
		p.position -= (penetration - EPSILON) * plane.normal;
	}

	return p;
}

bool isParticleAtRest(const Particle& p,
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
		float d = calculatePenetrationDepth(p, *plane);

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

	if(p.is_rest && !isParticleAtRest(p, scene, planes))
	{
		p.is_rest = false;
	}

	if(!p.is_rest)
	{
		while(timestep_remaining > EPSILON)
		{
			float timestep = timestep_remaining;
			Particle p_new = integrateParticle(p, scene, timestep);

			std::vector<Contact> contacts = detectCollisions(p, p_new, planes);

			if(!contacts.empty())
			{
				float earliest_time = contacts[0].time_of_impact;
				timestep *= earliest_time;
				p_new = integrateParticle(p, scene, timestep);
				p_new = handleMultipleCollisions(p_new, contacts);
			}

			timestep_remaining -= timestep;
			p = p_new;

			if(timestep < EPSILON)
			{
				break;
			}

			if(isParticleAtRest(p, scene, planes))
			{
				p.is_rest = true;
				break;
			}
		}
	}

	DrawModel(sphere_model,
			  {p.position.x(), p.position.y(), p.position.z()},
			  p.radius,
			  p.is_rest ? GRAY : WHITE);

	if(IsKeyDown(KEY_R))
	{
		p.position = {0, 5, 0};
		p.velocity = {9, 0, 9};
	}

	if(IsKeyDown(KEY_SPACE))
	{
		const auto c = graphics::graphics::camera;
		const auto origin = Eigen::Vector3f(c.position.x, c.position.y, c.position.z);
		const auto target = Eigen::Vector3f(c.target.x, c.target.y, c.target.z);
		const auto aa = origin - target;
		p.position = origin;
		p.velocity = (target - origin + Eigen::Vector3f::UnitY() * 2.0f).normalized() * 10.0f;
	}
}

int main(void)
{
	std::random_device rd;
	std::mt19937 e2(rd());
	std::uniform_real_distribution<float> distribution(-5.0, 5.0);

	std::cout << "hello from " << __FILE__ << std::endl;

	Scene scene;
	scene.gravity = -9.8 * Eigen::Vector3f::UnitY();
	scene.timestep = 1.0f / 60;
	scene.drawHZ = 60;
	ecs.set<Scene>(scene); // scene as a global singleton

	ecs.import <graphics::graphics>();
	graphics::graphics::init_window(ecs, CANVAS_WIDTH, CANVAS_HEIGHT, "Simulator");
	ecs.progress();

	flecs::log::set_level(0);

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

	add_particle({1, 5, 0}, {5, -10, 10});
	// add_particle({-1, 5, 0}, {5, 0, -5});

	add_plane(Eigen::Vector3f(5, 5, 0), {-1, 0, 0});
	add_plane(Eigen::Vector3f(-5, 5, 0), {1, 0, 0});
	add_plane(Eigen::Vector3f(0, 2.01, 0), {0.3, 1, 0});
	add_plane(Eigen::Vector3f(0, 5, 5), {0, 0, -1});
	add_plane(Eigen::Vector3f(0, 5, -5), {0, 0, 1});

	ecs.system<Particle, const Scene>("Simulator")
		.term_at(1)
		.singleton()
		.kind(flecs::OnUpdate)
		.each(simulate_particle);

	ecs.system<Plane>("DrawPlane").kind(flecs::OnUpdate).each(draw_plane);

	// simulator should own the main loop, not renderer
	graphics::graphics::run_main_loop(ecs, []() {
		// std::cout << "additional main loop callback" << std::endl;
	});

	// code below will be executed once the main loop break
	std::cout << "Bye!" << std::endl;

	return 0;
}
