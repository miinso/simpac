#include "flecs.h"
// #include "cglm.h"
// #include "deps/flecs_components_transform.h"
#include "flecs_components_physics.h"
#include "flecs_components_transform.h"

#include <iostream>
#include <vector>

using namespace flecs::components;

// Shortcuts to imported components
using Position = transform::Position3;
using Rotation = transform::Rotation3;
using Velocity = physics::Velocity3;

// Tag types
struct Eats { };
struct Apples { };

struct Mass {
	float value;
};

struct Vertex {
	vec3 position;
};

struct Edge {
	int vertex_index_a;
	int vertex_index_b;
};

struct Face {
	int vertex_index_a;
	int vertex_index_b;
	int vertex_index_c;
};

struct Mesh {
	std::vector<Vertex> vertices;
	std::vector<Edge> edges;
	std::vector<Face> faces;
	// vertex normal, face normal ...
};

struct DistanceConstraint {
	float rest_distance;
	float stiffness;
	float lambda;
};

struct VolumeConstraint {
	float rest_volume;
	float stiffness;
	float lambda;
};

void TestSystem(ecs_iter_t* it) { }

int main(int, char*[])
{
	// Create the world
	flecs::world world;

	// Register system
	world.system<Position, Velocity>().each([](Position& p, Velocity& v) {
		p.x += v.x;
		p.y += v.y;
	});

	world.system<Position, Velocity>().iter([](flecs::iter& it, Position* p, Velocity* v) {
		for(auto i : it)
		{
			std::cout << "Moved " << it.entity(i).name() << " to {" << p[i].x << ", " << p[i].y
					  << "}" << std::endl;
		}
	});

	// Create an entity with name Bob, add Position and food preference
	flecs::entity Bob = world
							.entity("Bob")
							// .set(Position{0, 0})
							.set<Position>({0, 0, 0})
							.set(Velocity{1, 2})
							.add<Eats, Apples>();

	// Show us what you got
	std::cout << Bob.name() << "'s got [" << Bob.type().str() << "]\n";

	// Run systems twice. Usually this function is called once per frame
	world.progress();
	world.progress();

	// See if Bob has moved (he has)
	const Position* p = Bob.get<Position>();
	std::cout << Bob.name() << "'s position is {" << p->x << ", " << p->y << ", " << p->z << "}\n";

	// Output
	//  Bob's got [Position, Velocity, (Identifier,Name), (Eats,Apples)]
	//  Bob's position is {2, 4}

	world.set_target_fps(1);
	while(world.progress())
	{
	}

	ECS_SYSTEM(world,
			   TestSystem,
			   EcsOnUpdate,
			   DistanceConstraint,
			   Mass,
			   flecs::In,
			   flecs::Entity,
			   flecs::Entity,
			   flecs::In,
			   Mass,
			   Mass,
			   flecs::InOut,
			   Position,
			   Position);

	// ecs_query_t *q_distance = ecs_query_init(world, &(ecs_query_desc_t) {
	// 	.filter.terms = {
	// 		{ecs_id(DistanceConstraint)},
	// 		{ecs_id(Mass)}
	// 	}
	// });

	world.system<DistanceConstraint, Mass, Mass, Position, Position>("DistanceConstraintSystem")
		.term<flecs::entity>(1)
		.in()
		.term<flecs::entity>(2)
		.in()
		.each([](DistanceConstraint& dc,
				 Mass& m1,
				 Mass& m2,
				 Position& p1,
				 Position& p2,
				 flecs::entity e1,
				 flecs::entity e2) {
					auto aa = m1.value + m2.value;
		});
}