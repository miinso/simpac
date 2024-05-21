#include "flecs.h"

#include <iostream>

// Component types
struct Position {
	double x;
	double y;
};

struct Velocity {
	double x;
	double y;
};

// Tag types
struct Eats { };
struct Apples { };

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
	flecs::entity Bob =
		world.entity("Bob").set(Position{0, 0}).set(Velocity{1, 2}).add<Eats, Apples>();

	// Show us what you got
	std::cout << Bob.name() << "'s got [" << Bob.type().str() << "]\n";

	// Run systems twice. Usually this function is called once per frame
	world.progress();
	world.progress();

	// See if Bob has moved (he has)
	const Position* p = Bob.get<Position>();
	std::cout << Bob.name() << "'s position is {" << p->x << ", " << p->y << "}\n";

	// Output
	//  Bob's got [Position, Velocity, (Identifier,Name), (Eats,Apples)]
	//  Bob's position is {2, 4}

	world.set_target_fps(1);
	while(world.progress())
	{
	}
}