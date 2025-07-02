#include "collider.h"
#include "types.h"
#include <broad.h>
#include <flecs.h>
#include <raylib.h>
#include <vector>

using namespace phys::pbd;
// using namespace phys::pbd::rigidbody;

inline void register_prefabs(flecs::world &ecs)
{
	ecs.prefab<prefabs::RigidBody>()
	    .add<RigidBody>()

	    .add<Position>()
	    .add<Orientation>()
	    .add<LinearVelocity>()
	    .add<AngularVelocity>()

	    .add<Position0>()
	    .add<Orientation0>()
	    .add<LinearVelocity0>()
	    .add<AngularVelocity0>()

	    .add<OldPosition>()
	    .add<OldOrientation>()
	    .add<OldLinearVelocity>()
	    .add<OldAngularVelocity>()

	    .add<LinearForce>()
	    .add<AngularForce>()        // torque
	    .add<Mass>()
	    .add<InverseMass>()
	    .add<LocalInertia>()
	    .add<LocalInverseInertia>()
	    // .add<WorldInertia> ()
	    // .add<WorldInverseInertia> ()
	    // .add<Mesh>()
	    // .add<PhysicsMesh>();
	    .add<Forces>()        // list of external forces

	    // collision
	    .add<IsActive>()        // a body starts with active collision state
	    .add<BoundingSphere>()
	    .add<Colliders>()
	    .add<DeactivationTime>();
}

flecs::entity add_rigid_body(flecs::world      &ecs,
                             const Vector3r    &x0      = Vector3r::Zero(),
                             const Quaternionr &q0      = Quaternionr::Identity(),
                             const Vector3r    &v0      = Vector3r::Zero(),
                             const Vector3r    &omega0  = Vector3r::Zero(),
                             Real               mass    = 1.0f,
                             const Matrix3r    &inertia = Matrix3r::Identity())
{
	// TODO: inertia should be set when a physics_mesh is attached to this
	// entity do closed form thing or volume integration.
	Matrix3r inv_inertia = inertia.inverse();

	return ecs.entity()
	    .is_a<prefabs::RigidBody>()

	    .set<Position>({x0})
	    .set<Orientation>({q0})
	    .set<LinearVelocity>({v0})
	    .set<AngularVelocity>({omega0})

	    .set<Position0>({x0})
	    .set<Orientation0>({q0})
	    .set<LinearVelocity0>({v0})
	    .set<AngularVelocity0>({omega0})

	    .set<OldPosition>({x0})
	    .set<OldOrientation>({q0})
	    .set<OldLinearVelocity>({v0})
	    .set<OldAngularVelocity>({omega0})

	    .set<LinearForce>({Vector3r::Zero()})
	    .set<AngularForce>({Vector3r::Zero()})
	    .set<Mass>({mass})
	    .set<InverseMass>({1.0f / mass})
	    .set<LocalInertia>({inertia})        // should be initialized when `Collider` component is attached to this entity.
	    .set<LocalInverseInertia>({inv_inertia});
}

// flecs::entity add_rigidbody() {}
// flecs::entity add_constraint() {}
// void draw_rigidbody() {}

// Collider entity_create_sphere_collider (Real radius) {
//     return collider_sphere_create (radius);
// }

// gameobject <-child, parent-> gameobject
// gameobject <-entity
// rb.set<Collider>(c1);
// rb.set<Collider>(c2);
// rb.set<Collider>(c3);

// entity = rb.parent
// list<entity> = rb.children

// rb.child_of(child_rb);

// collider.set<Collider>(c1).child_of(rb);

// rb.children

// void iterate_tree(flecs::entity e, Position p_parent = {0, 0}) {
//     // Print hierarchical name of entity & the entity type
//     std::cout << e.path() << " [" << e.type().str() << "]\n";

//     // Get entity position
//     const Position *p = e.get<Position>();

//     // Calculate actual position
//     Position p_actual = {p->x + p_parent.x, p->y + p_parent.y};
//     std::cout << "{" << p_actual.x << ", " << p_actual.y << "}\n\n";

//     // Iterate children recursively
//     e.children([&](flecs::entity child) {
//         iterate_tree(child, p_actual);
//     });
// }

Collider collider_convex_hull_from_mesh(const Mesh &mesh, const Vector3r &scale)
{
	std::vector<Vector3r> vertices;
	vertices.reserve(mesh.vertexCount);

	for (int i = 0; i < mesh.vertexCount; ++i)
	{
		Vector3r position;
		position.x() = static_cast<Real>(mesh.vertices[i * 3]) * scale.x();
		position.y() = static_cast<Real>(mesh.vertices[i * 3 + 1]) * scale.y();
		position.z() = static_cast<Real>(mesh.vertices[i * 3 + 2]) * scale.z();
		vertices.push_back(position);
	}

	std::vector<unsigned int> indices;
	indices.reserve(mesh.triangleCount * 3);
	for (int i = 0; i < mesh.triangleCount * 3; ++i)
	{
		indices.push_back(static_cast<unsigned int>(mesh.indices[i]));
	}

	return collider_convex_hull_create(vertices, indices);
}

std::vector<Collider> entity_create_sphere_collider(Real radius)
{
	std::vector<Collider> colliders;
	colliders.push_back(collider_sphere_create(radius));
	return colliders;
}

std::vector<Collider> entity_create_mesh_collider(const Mesh &mesh, const Vector3r &scale)
{
	std::vector<Collider> colliders;
	colliders.push_back(collider_convex_hull_from_mesh(mesh, scale));
	return colliders;
}

std::vector<Collider> example_util_create_sphere_convex_hull_array(Real radius)
{
	Collider collider = collider_sphere_create(radius);

	std::vector<Collider> colliders;
	colliders.push_back(collider);

	return colliders;
}

void example_util_throw_object(flecs::world &ecs)
{
	auto e1 = add_rigid_body(ecs);

	// generate random mesh
	int r         = rand();
	int is_sphere = 0;

	Mesh m;

	if (true)
	{
		m = GenMeshCube(1, 1, 1);
	}
	else if (r % 4 == 1)
	{
		m = GenMeshCone(0.5, 1, 8);
	}
	else if (r % 4 == 2)
	{
		m = GenMeshCylinder(0.5, 1, 8);
	}
	else
	{
		is_sphere = 1;
		m         = GenMeshSphere(0.5, 8, 8);
	}
	auto model = LoadModelFromMesh(m);

	// e1.set<Mesh0>({m});

	// generate colliders from the mesh
	Vector3r              scale;
	std::vector<Collider> colliders;

	if (is_sphere)
	{
		Real radius = 1.0;
		scale       = Vector3r::Ones() * radius;
		colliders   = entity_create_sphere_collider(radius);
	}
	else
	{
		scale     = Vector3r::Ones();
		colliders = entity_create_mesh_collider(m, scale);
	}
	e1.set<Colliders>({colliders});

	// set bounding volume
	auto radius = colliders_get_bounding_sphere_radius(colliders);
	e1.set<BoundingSphere>({radius});

	// set inertia
	auto inertia =
	    colliders_get_default_inertia_tensor(colliders, e1.get<Mass>()->value);
	e1.set<LocalInertia>({inertia});
	e1.set<LocalInverseInertia>({inertia.inverse()});

	// set initial condition
	e1.get_mut<LinearVelocity>()->value  = Vector3r(0.0, 15.0, 0.0);
	e1.get_mut<AngularVelocity>()->value = {0, 2, 10};
}

void example_util_install_floor(flecs::world &ecs)
{
	Vector3r floor_scale  = {1, 1, 1};
	auto     floor_entity = add_rigid_body(ecs);

	// visual mesh
	auto cube_mesh  = GenMeshCube(5, 0.5, 5);
	auto cube_model = LoadModelFromMesh(cube_mesh);
	floor_entity.set<RenderMesh>({cube_model});

	// set collision mesh
	auto floor_colliders = entity_create_mesh_collider(cube_mesh, floor_scale);
	floor_entity.set<Colliders>({floor_colliders});

	// set bounding volume
	auto radius = colliders_get_bounding_sphere_radius(floor_colliders);
	floor_entity.set<BoundingSphere>({radius});

	// set inertia
	auto inertia = colliders_get_default_inertia_tensor(
	    floor_colliders, floor_entity.get<Mass>()->value);
	floor_entity.set<LocalInertia>({inertia});
	floor_entity.set<LocalInverseInertia>({inertia.inverse()});

	// floor is fixed
	floor_entity.add<IsPinned>();
}

flecs::entity add_box_object(flecs::world &ecs)
{
	Vector3r floor_scale = {1, 1, 1};
	auto     box_entity  = add_rigid_body(ecs);

	// visual mesh
	auto cube_mesh = GenMeshCube(1, 1, 1);
	auto model     = LoadModelFromMesh(cube_mesh);
	// model.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = RED;
	model.materials[0] = LoadMaterialDefault();

	box_entity.set<RenderMesh>({model});

	// set collision mesh
	auto box_colliders = entity_create_mesh_collider(cube_mesh, floor_scale);
	box_entity.set<Colliders>({box_colliders});

	// mesh -> render_mesh + collider?

	// set bounding volume
	auto radius = colliders_get_bounding_sphere_radius(box_colliders);
	box_entity.set<BoundingSphere>({radius});

	// set inertia
	auto inertia = colliders_get_default_inertia_tensor(
	    box_colliders, box_entity.get<Mass>()->value);
	box_entity.set<LocalInertia>({inertia});
	box_entity.set<LocalInverseInertia>({inertia.inverse()});

	// entity_create_box_collider
	// entity_create_sphere_collider
	// entity_create_capsule_collider
	// entity_create_mesh_collider

	// rigidbody + box collider = box object
	// a gameobject = an entity with:
	//   - rigidbody component
	//   - collider component
	//
	//
	return box_entity;
}

void simulate(flecs::world &ecs, Real dt)
{
	// find all rigidbodies
	auto get_all_rigidbodies =
	    ecs.query_builder<Position>().with<RigidBody>().cached().build();
	std::vector<flecs::entity> entities;
	entities.reserve(get_all_rigidbodies.count());

	get_all_rigidbodies.each(
	    [&](flecs::entity e, Position &x) { entities.push_back(e); });

	// auto broad_collision_pairs = broad_get_collision_pairs (entities);

	// // do the island sleep thing
	// auto simulation_islands =
	// broad_collect_simulation_islands (entities, broad_collision_pairs);
	// for (size_t j = 0; j < simulation_islands.size (); ++j) {
	//     auto simulation_island = simulation_islands[j];

	//     bool all_inactive = true;
	//     for (size_t k = 0; k < simulation_island.size (); ++k) {
	//         auto e = simulation_island[k];

	//         auto linear_velocity_norm = e.get<LinearVelocity> ()->value.norm ();
	//         auto angular_velocity_norm = e.get<AngularVelocity> ()->value.norm ();
	//         if (linear_velocity_norm < LINEAR_SLEEPING_THRESHOLD &&
	//         angular_velocity_norm < ANGULAR_SLEEPING_THRESHOLD) {
	//             e.get_mut<DeactivationTime> ()->value += dt;
	//         } else {
	//             e.get_mut<DeactivationTime> ()->value = 0.0;
	//         }

	//         if (e.get_mut<DeactivationTime> ()->value < DEACTIVATION_TIME_TO_BE_INACTIVE) {
	//             all_inactive = false;
	//         }
	//     }

	//     // we only set entities to inactive if the whole island is inactive!
	//     for (size_t k = 0; k < simulation_island.size (); ++k) {
	//         auto e                        = simulation_island[k];
	//         e.get_mut<IsActive> ()->value = !all_inactive;
	//     }
	// }
	// broad_simulation_islands_destroy(simulation_islands);

	// integrate

	// collect collision

	// constraint solve

	// velocity update

	// velocity pass
}