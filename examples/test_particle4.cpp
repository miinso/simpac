#include "Eigen/Dense"
#include "flecs.h"
#include "graphics_module.h"
#include <array>
#include <chrono>
#include <iostream>
#include <random>

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

constexpr int CANVAS_WIDTH = 800;
constexpr int CANVAS_HEIGHT = 600;
constexpr float EPSILON = 1e-6f;
constexpr float COLLISION_THRESHOLD = 0.01f;
constexpr float RESTING_VELOCITY_THRESHOLD = 0.01f;
constexpr float RESTING_DISTANCE_THRESHOLD = 0.01f;
constexpr float RESTING_FORCE_THRESHOLD = 0.01f;

Sound note0;
Sound note1;
Sound note2;
Sound note3;
Sound note4;

const int SEQUENCE_LENGTH = 9;
Sound* sequence[SEQUENCE_LENGTH];

int collision_sfx_count = 0;

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
	bool is_visible;
	bool is_audible;
};

struct Scene {
	Eigen::Vector3f gravity;
	float air_resistance;
	float simulation_time_step;
	float accumulated_time;

	double frame_time;
	double sim_time;

	float min_frequency = 30;
	float max_frequency = 10000;
	float frequency_slider;
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

// utils
Vector3 e2r(const Eigen::Vector3f& v) {
	return {v.x(), v.y(), v.z()};
}

Eigen::Vector3f r2e(const Vector3& v) {
	return Eigen::Vector3f(v.x, v.y, v.z);
}

float LinearToLog(float value, float min, float max) {
	float minLog = log10(min);
	float maxLog = log10(max);
	return (log10(value) - minLog) / (maxLog - minLog);
}

float LogToLinear(float value, float min, float max) {
	float minLog = log10(min);
	float maxLog = log10(max);
	return pow(10, value * (maxLog - minLog) + minLog);
}

void handle_particle_plane_collisions(
	flecs::iter& it, size_t index, Position& p, Velocity& v, ParticleProperties& props) {
	auto scene = it.world().get<Scene>();

	it.world().each(
		[&](flecs::entity e, const Position& plane_pos, const PlaneProperties& plane_props) {
			float penetration = (p.value - plane_pos.value).dot(plane_props.normal) - props.radius;

			if(penetration < 0) {
				// Collision response
				Eigen::Vector3f v_n = v.value.dot(plane_props.normal) * plane_props.normal;
				Eigen::Vector3f v_t = v.value - v_n;

				v_n = -props.restitution * v_n;

				float friction_magnitude = std::min(plane_props.friction * v_n.norm(), v_t.norm());
				v_t -= friction_magnitude * v_t.normalized();

				v.value = v_n + v_t;

				p.value -= (penetration - EPSILON) * plane_props.normal;

				// Update collision count
				auto mutable_plane = e.get_mut<PlaneProperties>();
				mutable_plane->collision_count++;
				if(mutable_plane->is_audible) {
					collision_sfx_count++;
					int index = collision_sfx_count % SEQUENCE_LENGTH;
					// sequence[index]->play();
					// std::cout << "play" << std::endl;
					if(sequence[index] != nullptr) {
						PlaySound(*sequence[index]);
					}
				}
			}
		});
}

void check_particle_rest_state(
	flecs::iter& it, size_t index, Position& p, Velocity& v, ParticleProperties& props) {
	auto scene = it.world().get<Scene>();

	if(v.value.norm() < RESTING_VELOCITY_THRESHOLD) {
		bool at_rest = false;
		it.world().each([&](const Position& plane_pos, const PlaneProperties& plane_props) {
			float d = (p.value - plane_pos.value).dot(plane_props.normal) - props.radius;
			if(std::abs(d) < RESTING_DISTANCE_THRESHOLD) {
				Eigen::Vector3f total_force = scene->gravity * props.mass;
				float normal_force = total_force.dot(plane_props.normal);
				if(normal_force < -RESTING_FORCE_THRESHOLD) {
					Eigen::Vector3f tangential_force =
						total_force - normal_force * plane_props.normal;
					if(tangential_force.norm() <= plane_props.friction * std::abs(normal_force)) {
						at_rest = true;
					}
				}
			}
		});
		props.is_rest = at_rest;
	} else {
		props.is_rest = false;
	}
}

void draw_particles(flecs::iter& it,
					size_t index,
					const Position& p,
					const ParticleProperties& props) {
	auto res = it.world().get<SimulationResources>();
	DrawModel(res->sphere_model, e2r(p.value), props.radius, props.is_rest ? GRAY : WHITE);
}

void draw_planes(flecs::iter& it, size_t index, const Position& p, const PlaneProperties& props) {
	if(props.is_visible == false) {
		return;
	}

	auto res = it.world().get<SimulationResources>();
	const std::array<Color, 4> PLANE_COLORS = {Color{66, 133, 244, 255},
											   Color{234, 67, 53, 255},
											   Color{251, 188, 5, 255},
											   Color{52, 168, 83, 255}};

	Eigen::Vector3f position = p.value;
	Eigen::Vector3f normal = props.normal.normalized();

	Eigen::Vector3f up = Eigen::Vector3f::UnitY();
	if(normal.dot(up) > 0.99f || normal.dot(up) < -0.99f) {
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

void reset_particles(flecs::iter& it) {
	const auto& camera = graphics::graphics::camera;

	auto add_particle = [&](const Eigen::Vector3f& pos, const Eigen::Vector3f& vel) {
		it.world()
			.entity()
			.add<Particle>()
			.set<Position>({pos})
			.set<Velocity>({vel})
			.set<ParticleProperties>({0.5f, 1.0f, 0.3f, 0.7f, false});
	};

	auto add_plane =
		[&](const Eigen::Vector3f& pos, const Eigen::Vector3f& normal, const bool visible = true) {
			it.world().entity().add<Plane>().set<Position>({pos}).set<PlaneProperties>(
				{normal, 0.3f, 0.7f, 0, visible});
		};

	if(IsKeyPressed(KEY_R)) {
		it.world().each([&](flecs::entity e, const Particle&) { e.destruct(); });
	}

	if(IsKeyPressed(KEY_DELETE)) {
		PlaySound(note0);
	}

	if(IsKeyPressed(KEY_ONE)) {
		it.world().each([&](flecs::entity e, const Particle&) { e.destruct(); });

		it.world()
			.entity()
			.add<Particle>()
			.set<Position>({{0, 5, 0}})
			.set<Velocity>({{10, 0, 0}})
			.set<ParticleProperties>({0.5f, 1.0f, 0.3f, 0.7f, false});
	}

	if(IsKeyPressed(KEY_TWO)) {
		it.world().each([&](flecs::entity e, const Particle&) { e.destruct(); });

		it.world()
			.entity()
			.add<Particle>()
			.set<Position>({{0, 5, 0}})
			.set<Velocity>({{10, 0, 10}})
			.set<ParticleProperties>({0.5f, 1.0f, 0.3f, 0.7f, false});
	}

	if(IsKeyPressed(KEY_THREE)) {
		it.world().each([&](flecs::entity e, const Particle&) { e.destruct(); });

		it.world()
			.entity()
			.add<Particle>()
			.set<Position>({{0, 5, 0}})
			.set<Velocity>({{0, 0, 10}})
			.set<ParticleProperties>({0.5f, 1.0f, 0.3f, 0.7f, false});
	}

	if(IsKeyPressed(KEY_FOUR)) {
		it.world().each([&](flecs::entity e, const Particle&) { e.destruct(); });

		it.world()
			.entity()
			.add<Particle>()
			.set<Position>({{0, 5, 0}})
			.set<Velocity>({{-10, 0, 10}})
			.set<ParticleProperties>({0.5f, 1.0f, 0.3f, 0.7f, false});
	}

	if(IsKeyPressed(KEY_LEFT_BRACKET)) {
		it.world().each([&](flecs::entity e, const Particle&) { e.destruct(); });
		it.world().each([&](flecs::entity e, const Plane&) { e.destruct(); });

		add_particle({1, 5, 0}, {10, 0, 10});
		add_plane({5, 5, 0}, {-1, 0, 0});
		add_plane({-5, 5, 0}, {1, 0, 0});
		add_plane({0, 0, 0}, {0, 1, 0}, false); // ground
		add_plane({0, 10, 0}, {0, -1, 0}, false); // ceiling
		add_plane({0, 5, 5}, {0, 0, -1});
		add_plane({0, 5, -5}, {0, 0, 1});
	}

	if(IsKeyPressed(KEY_RIGHT_BRACKET)) {
		it.world().each([&](flecs::entity e, const Particle&) { e.destruct(); });
		it.world().each([&](flecs::entity e, const Plane&) { e.destruct(); });

		add_particle({1, 5, 0}, {10, 0, 10});
		// add_plane({5, 5, 0}, {-1, 0, 0});
		// add_plane({-5, 5, 0}, {1, 0, 0});

		add_plane({-3, 0, 0}, {1, 1, 0}, true); // ground
		add_plane({3, 0, 0}, {-1, 1, 0}, true); // ceiling

		// add_plane({0, 0, 0}, {-1, 1, 0});
		// add_plane({0, 0, 0}, {0, 1, -1});
	}
}

void create_particle_on_input(flecs::iter& it) {
	const auto& camera = graphics::graphics::camera;

	if(IsKeyPressed(KEY_SPACE)) {
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
	flecs::iter& it, size_t index, Position& p, Velocity& v, ParticleProperties& props) {
	auto world = it.world();

	world.each([&](flecs::entity other_entity,
				   const Position& other_p,
				   const ParticleProperties& other_props) {
		if(it.entity(index) == other_entity)
			return;

		Eigen::Vector3f delta = p.value - other_p.value;
		float distance = delta.norm();
		float min_distance = props.radius + other_props.radius;

		if(distance < min_distance) {
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

int main() {

	std::cout << "Hi from " << __FILE__ << std::endl;

	flecs::world ecs;

	ecs.component<Position>();
	ecs.component<Velocity>();
	ecs.component<ParticleProperties>();
	ecs.component<PlaneProperties>();
	ecs.component<Particle>();
	ecs.component<Plane>();

	ecs.set<Scene>({
		-9.8f * Eigen::Vector3f::UnitY(), // gravity
		0.5f, // air_resistance
		1.0f / 1000.0f,
		0.0f // accumulated_time
	});

	// Import and initialize graphics
	ecs.import <graphics::graphics>();
	graphics::graphics::init_window(ecs, CANVAS_WIDTH, CANVAS_HEIGHT, "Particle Simulator");
	InitAudioDevice();

	// Initialize simulation resources, must be called after graphics::init
	SimulationResources res;
	Image checked = GenImageChecked(6, 6, 1, 1, LIGHTGRAY, DARKGRAY);
	res.texture = LoadTextureFromImage(checked);
	UnloadImage(checked);

	res.plane_model = LoadModelFromMesh(GenMeshPlane(2, 2, 4, 3));
	res.plane_model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = res.texture;

	Mesh cubeMesh = GenMeshCube(2, 0.0f, 2);
	GenMeshTangents(&cubeMesh);
	res.cube_model = LoadModelFromMesh(cubeMesh);
	res.cube_model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = res.texture;

	res.sphere_model = LoadModelFromMesh(GenMeshSphere(1, 64, 64));
	res.sphere_model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = res.texture;
	ecs.set<SimulationResources>(res);

	note1 = LoadSound("resources/note1.ogg");
	note2 = LoadSound("resources/note2.ogg");
	note3 = LoadSound("resources/note3.ogg");
	note4 = LoadSound("resources/note4.ogg");
	note0 = LoadSound("resources/note0.ogg");
	sequence[0] = &note1;
	sequence[1] = &note2;
	sequence[2] = &note3;
	sequence[3] = &note2;
	sequence[4] = &note1;
	sequence[5] = &note4;
	sequence[6] = &note4;
	sequence[7] = &note4;
	sequence[8] = nullptr;

	// Create entities
	auto add_particle = [&](const Eigen::Vector3f& pos, const Eigen::Vector3f& vel) {
		ecs.entity()
			.add<Particle>()
			.set<Position>({pos})
			.set<Velocity>({vel})
			.set<ParticleProperties>({0.5f, 1.0f, 0.3f, 0.7f, false});
	};

	auto add_plane = [&](const Eigen::Vector3f& pos,
						 const Eigen::Vector3f& normal,
						 const bool visible = true,
						 const bool audible = true) {
		ecs.entity().add<Plane>().set<Position>({pos}).set<PlaneProperties>(
			{normal, 0.3f, 0.7f, 0, visible, audible});
	};

	add_particle({1, 5, 0}, {5, -10, 10});
	add_plane({5, 5, 0}, {-1, 0, 0});
	add_plane({-5, 5, 0}, {1, 0, 0});
	add_plane({0, 0, 0}, {0, 1, 0}, false, false); // ground
	add_plane({0, 15, 0}, {0, -1, 0}, false); // ceiling
	add_plane({0, 5, 5}, {0, 0, -1});
	add_plane({0, 5, -5}, {0, 0, 1});

	ecs.system("CreateParticleOnInput").kind(flecs::PreUpdate).run(create_particle_on_input);
	ecs.system("ResetParticles").kind(flecs::PreUpdate).run(reset_particles);

	auto simulate_particles_system =
		ecs.system<Position, Velocity, const ParticleProperties>().each(
			[](flecs::iter& it,
			   size_t index,
			   Position& p,
			   Velocity& v,
			   const ParticleProperties& props) {
				auto scene = it.world().get<Scene>();
				if(!props.is_rest) {
					v.value += it.delta_time() * (1.0f / props.mass) *
							   (scene->gravity + scene->air_resistance * -v.value);
					p.value += it.delta_time() * v.value;
				}
			});

	auto handle_collisions_system = ecs.system<Position, Velocity, ParticleProperties>().each(
		[](flecs::iter& it, size_t index, Position& p, Velocity& v, ParticleProperties& props) {
			handle_particle_plane_collisions(it, index, p, v, props);
			handle_particle_particle_collisions(it, index, p, v, props);
		});

	auto check_particle_rest_state_system =
		ecs.system<Position, Velocity, ParticleProperties>().each(
			[](flecs::iter& it, size_t index, Position& p, Velocity& v, ParticleProperties& props) {
				check_particle_rest_state(it, index, p, v, props);
			});

	ecs.system("physics::step").kind(flecs::OnUpdate).run([&](flecs::iter& it) {
		auto scene = it.world().get_mut<Scene>();
		scene->accumulated_time += it.delta_time();

		auto start_time = std::chrono::high_resolution_clock::now();

		// std::cout << "onupdate" << std::endl;
		while(scene->accumulated_time >= scene->simulation_time_step) {
			simulate_particles_system.run();
			handle_collisions_system.run();
			check_particle_rest_state_system.run();
			scene->accumulated_time -= scene->simulation_time_step;
			// std::cout << "onphysics!!" << std::endl;
		}

		auto end_time = std::chrono::high_resolution_clock::now();
		scene->sim_time = std::chrono::duration<double, std::milli>(end_time - start_time).count();
	});

	ecs.system("graphics::begin").kind(flecs::PostUpdate).run([](flecs::iter& it) {
		it.world().get_mut<Scene>()->frame_time = 0.0;
		BeginDrawing();
	});

	ecs.system("graphics::clear").kind(flecs::PostUpdate).run([](flecs::iter& it) {
		ClearBackground(RAYWHITE);
	});

	ecs.system("graphics::begin_mode_3d").kind(flecs::PostUpdate).run([](flecs::iter& it) {
		BeginMode3D(graphics::graphics::camera);
		DrawGrid(12, 10.0f / 12);
	});

	ecs.system<const Position, const ParticleProperties>("DrawParticles")
		.kind(flecs::PostUpdate)
		.each(draw_particles);

	ecs.system<const Position, const PlaneProperties>("DrawPlanes")
		.kind(flecs::PostUpdate)
		.each(draw_planes);

	ecs.system("graphics::end_mode_3d").kind(flecs::PostUpdate).run([](flecs::iter& it) {
		EndMode3D();
		DrawFPS(20, 20);
	});

	ecs.system("DisplayTimingInfo").kind(flecs::PostUpdate).run([](flecs::iter& it) {
		auto scene = it.world().get_mut<Scene>();
		scene->frame_time = GetFrameTime() * 1000.0; // to milliseconds
		// auto scene = it.world().get<Scene>();
		DrawText(TextFormat("Simulation Took: %.2f ms", scene->sim_time), 20, 60, 20, LIME);
		DrawText(TextFormat("Frame Took: %.2f ms", scene->frame_time), 20, 80, 20, LIME);
		DrawText(TextFormat("Simulation Timestep: %.7f ms", scene->simulation_time_step),
				 20,
				 120,
				 20,
				 LIME);
	});

	ecs.system("HandleTimeStepControl").kind(flecs::PostUpdate).run([](flecs::iter& it) {
		auto scene = it.world().get_mut<Scene>();

		float logValue =
			LinearToLog(scene->frequency_slider, scene->min_frequency, scene->max_frequency);

		const float adjustment_factor = 0.2f;
		if(IsKeyPressed(KEY_PERIOD)) {
			logValue = fmin(logValue + adjustment_factor, 1.0f);
		}
		if(IsKeyPressed(KEY_COMMA)) {
			logValue = fmax(logValue - adjustment_factor, 0.0f);
		}

		// timestep control
		GuiSlider(Rectangle{20, 140, 200, 20}, nullptr, nullptr, &logValue, 0.0f, 1.0f);
		scene->frequency_slider = LogToLinear(logValue, scene->min_frequency, scene->max_frequency);
		scene->simulation_time_step = 1.0f / scene->frequency_slider;

		float airvalue = scene->air_resistance;
		DrawText(TextFormat("Air Resistance: %.2f", scene->air_resistance), 20, 180, 20, LIME);
		// air resistance control
		GuiSlider(Rectangle{20, 200, 200, 20}, nullptr, nullptr, &airvalue, 0.0f, 1.0f);
		scene->air_resistance = airvalue;
	});

	ecs.system("graphics::end").kind(flecs::PostUpdate).run([](flecs::iter& it) { EndDrawing(); });

	// main loop
	graphics::graphics::run_main_loop(ecs, []() {
		// callback if needed
	});

	std::cout << "Simulation ended." << std::endl;
	return 0;
}