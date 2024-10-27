#include "hw2aux.h"
#include <iostream>

Vector3 e2r(const Eigen::Vector3f& v) {
	return {v.x(), v.y(), v.z()};
}

Eigen::Vector3f r2e(const Vector3& v) {
	return Eigen::Vector3f(v.x, v.y, v.z);
}
Matrix e2r_matrix(const Eigen::Matrix4f& m) {
	return {m(0, 0),
			m(0, 1),
			m(0, 2),
			m(0, 3),
			m(1, 0),
			m(1, 1),
			m(1, 2),
			m(1, 3),
			m(2, 0),
			m(2, 1),
			m(2, 2),
			m(2, 3),
			m(3, 0),
			m(3, 1),
			m(3, 2),
			m(3, 3)};
}

////////////////////
// body system work
// Helper function to register Eigen::Vector3f
void register_vector3f(flecs::world& world) {
	world.component<Eigen::Vector3f>().member<float>("x").member<float>("y").member<float>("z");
}

// Helper function to register Eigen::Quaternionf
void register_quaternionf(flecs::world& world) {
	world.component<Eigen::Quaternionf>()
		.member<float>("x")
		.member<float>("y")
		.member<float>("z")
		.member<float>("w");
}

void init_body_system(flecs::world& world) {
	register_vector3f(world);
	register_quaternionf(world);

	world.component<aabb>().member<Eigen::Vector3f>("min").member<Eigen::Vector3f>("max");

	world.component<trimesh>();

	world.component<body>()
		.member<Eigen::Vector3f>("position")
		.member<Eigen::Quaternionf>("orientation")
		.member<Eigen::Vector3f>("scale");

	world.system<body, aabb, trimesh>("update_aabb")
		.kind(flecs::OnUpdate)
		.each([](body& b, aabb& box, const trimesh& t) { body::update_aabb(b, t, box); });
}

static Mesh GenMeshTriangle(float size) {
	Mesh mesh = {0};
	mesh.triangleCount = 1;
	mesh.vertexCount = mesh.triangleCount * 3;
	mesh.vertices = (float*)MemAlloc(mesh.vertexCount * 3 * sizeof(float));
	mesh.texcoords = (float*)MemAlloc(mesh.vertexCount * 2 * sizeof(float));
	mesh.normals = (float*)MemAlloc(mesh.vertexCount * 3 * sizeof(float));

	// Scale the triangle based on the size parameter
	float halfSize = size * 0.5f;

	// Vertex 1: (0, 0, 0)
	mesh.vertices[0] = -halfSize;
	mesh.vertices[1] = 0;
	mesh.vertices[2] = halfSize;
	mesh.normals[0] = 0;
	mesh.normals[1] = 1;
	mesh.normals[2] = 0;
	mesh.texcoords[0] = 0;
	mesh.texcoords[1] = 0;

	// Vertex 2: (size, 0, 0)
	mesh.vertices[3] = halfSize;
	mesh.vertices[4] = 0;
	mesh.vertices[5] = halfSize;
	mesh.normals[3] = 0;
	mesh.normals[4] = 1;
	mesh.normals[5] = 0;
	mesh.texcoords[2] = 1;
	mesh.texcoords[3] = 0;

	// Vertex 3: (size/2, 0, -size*sqrt(3)/2)
	mesh.vertices[6] = 0;
	mesh.vertices[7] = 0;
	mesh.vertices[8] = -halfSize;
	mesh.normals[6] = 0;
	mesh.normals[7] = 1;
	mesh.normals[8] = 0;
	mesh.texcoords[4] = 0.5f;
	mesh.texcoords[5] = 1;

	UploadMesh(&mesh, false);

	return mesh;
}

flecs::entity
create_body_from_mesh(flecs::world& world, const Mesh& mesh, const Eigen::Vector3f& position) {
	auto entity = world.entity();

	body b;
	b.position = position;
	b.orientation = Eigen::Quaternionf::Identity();
	b.scale = Eigen::Vector3f::Ones();
	entity.set<body>(b);

	aabb a;
	entity.set<aabb>(a);

	trimesh t;
	t.from_raylib(mesh);
	t.init();
	entity.set<trimesh>(t);

	return entity;
}

flecs::entity create_body_from_3d_file(flecs::world& world,
									   const char* filename,
									   const Eigen::Vector3f& position,
									   const Eigen::Quaternionf& orientation) {
	Model model = LoadModel(filename);

	for (int i = 0; i < model.meshCount; i++) {
		GenMeshTangents(&model.meshes[i]);
	}

	auto entity = create_body_from_mesh(world, model.meshes[0], position);

	entity.get_mut<body>()->orientation = orientation;
	entity.get_mut<body>()->scale = Eigen::Vector3f::Ones();

	aabb model_aabb;
	body::update_aabb(*entity.get<body>(), *entity.get<trimesh>(), model_aabb);
	entity.set<aabb>(model_aabb);

	entity.get_mut<trimesh>()->model = model;

	return entity;
}

flecs::entity create_plane_body(flecs::world& world,
								float width,
								float length,
								const Eigen::Vector3f& position,
								const Eigen::Vector3f& normal) {
	Mesh plane_mesh = GenMeshPlane(width, length, 4, 4);
	auto entity = create_body_from_mesh(world, plane_mesh, position);

	Eigen::Vector3f up = Eigen::Vector3f::UnitY();
	Eigen::Quaternionf rotation = Eigen::Quaternionf::FromTwoVectors(up, normal);
	entity.get_mut<body>()->orientation = rotation;

	// UnloadMesh(plane_mesh);

	return entity;
}

flecs::entity create_cube_body(flecs::world& world,
							   float width,
							   float height,
							   float depth,
							   const Eigen::Vector3f& position,
							   const Eigen::Quaternionf& orientation) {
	Mesh cube_mesh = GenMeshCube(width, height, depth);
	GenMeshTangents(&cube_mesh);
	auto entity = create_body_from_mesh(world, cube_mesh, position);

	entity.get_mut<body>()->orientation = orientation;

	entity.get_mut<body>()->scale = Eigen::Vector3f(width, height, depth);

	aabb cube_aabb;
	Eigen::Vector3f half_extents(width / 2, height / 2, depth / 2);
	cube_aabb.min = position - half_extents;
	cube_aabb.max = position + half_extents;
	entity.set<aabb>(cube_aabb);

	// UnloadMesh(cube_mesh);

	return entity;
}

flecs::entity create_sphere_body(flecs::world& world,
								 float radius,
								 const Eigen::Vector3f& position,
								 const Eigen::Quaternionf& orientation) {
	Mesh sphere_mesh = GenMeshSphere(radius, 4, 4);
	GenMeshTangents(&sphere_mesh);
	auto entity = create_body_from_mesh(world, sphere_mesh, position);

	entity.get_mut<body>()->orientation = orientation;

	entity.get_mut<body>()->scale =
		Eigen::Vector3f::Constant(radius * 2); // Diameter is twice the radius

	aabb sphere_aabb;
	Eigen::Vector3f extents = Eigen::Vector3f::Constant(radius);
	sphere_aabb.min = position - extents;
	sphere_aabb.max = position + extents;
	entity.set<aabb>(sphere_aabb);


	return entity;
}

flecs::entity create_poly_body(flecs::world& world,
							   int sides,
							   float radius,
							   const Eigen::Vector3f& position,
							   const Eigen::Quaternionf& orientation) {
	Mesh poly_mesh = GenMeshPoly(sides, radius);
	GenMeshTangents(&poly_mesh);
	auto entity = create_body_from_mesh(world, poly_mesh, position);

	// Set the orientation
	entity.get_mut<body>()->orientation = orientation;

	// Update the AABB
	aabb poly_aabb;
	poly_aabb.min = Eigen::Vector3f::Constant(std::numeric_limits<float>::max());
	poly_aabb.max = Eigen::Vector3f::Constant(std::numeric_limits<float>::lowest());

	auto& mesh = entity.get_mut<trimesh>()->vertices;
	for (const auto& v : mesh) {
		poly_aabb.min = poly_aabb.min.cwiseMin(v);
		poly_aabb.max = poly_aabb.max.cwiseMax(v);
	}

	poly_aabb.min += position;
	poly_aabb.max += position;
	entity.set<aabb>(poly_aabb);

	// UnloadMesh(poly_mesh);

	return entity;
}

flecs::entity create_triangle_body(flecs::world& world,
								   float size,
								   const Eigen::Vector3f& position,
								   const Eigen::Quaternionf& orientation) {
	Mesh triangle_mesh = GenMeshTriangle(size);
	GenMeshTangents(&triangle_mesh);
	auto entity = create_body_from_mesh(world, triangle_mesh, position);

	// Set the orientation
	entity.get_mut<body>()->orientation = orientation;

	// Update the AABB
	aabb triangle_aabb;
	triangle_aabb.min = Eigen::Vector3f::Constant(std::numeric_limits<float>::max());
	triangle_aabb.max = Eigen::Vector3f::Constant(std::numeric_limits<float>::lowest());

	auto& mesh = entity.get_mut<trimesh>()->vertices;
	for (const auto& v : mesh) {
		triangle_aabb.min = triangle_aabb.min.cwiseMin(v);
		triangle_aabb.max = triangle_aabb.max.cwiseMax(v);
	}

	triangle_aabb.min += position;
	triangle_aabb.max += position;
	entity.set<aabb>(triangle_aabb);

	// UnloadMesh(triangle_mesh);

	return entity;
}

void install_bodies(flecs::world& world) {
	init_body_system(world);

	// create_plane_body(world, 10, 10, Eigen::Vector3f(0, 0, 0), Eigen::Vector3f(0, 1, 0));
	// create_plane_body(world, 2, 2, Eigen::Vector3f(3, 3, 0), Eigen::Vector3f(-1, 0, 0));
	create_cube_body(
		world,
		2.3,
		0.2,
		1.7,
		Eigen::Vector3f(-1, 0, 0),
		Eigen::Quaternionf(Eigen::AngleAxisf(PI / 5.3, Eigen::Vector3f{-0.5, -0.2, -0.3})));
	create_sphere_body(world,
					   0.6f,
					   Eigen::Vector3f(0, 3, 0),
					   Eigen::Quaternionf(Eigen::AngleAxisf(PI / 9, Eigen::Vector3f::UnitX())));

	create_poly_body(
		world,
		5,
		2.0f,
		Eigen::Vector3f(2, 2, -2),
		Eigen::Quaternionf(Eigen::AngleAxisf(PI / 3.2, Eigen::Vector3f{0.1, 0.2, 0.3})));

	create_poly_body(
		world,
		4,
		2.0f,
		Eigen::Vector3f(2, 1.5, 2),
		Eigen::Quaternionf(Eigen::AngleAxisf(PI / 4.5, -Eigen::Vector3f{0.3, 0.1, 0.2})));

	create_poly_body(
		world,
		3,
		2.0f,
		Eigen::Vector3f(-2, 1.0, 2),
		Eigen::Quaternionf(Eigen::AngleAxisf(PI / 2.7, -Eigen::Vector3f{0.2, 0.1, 0.3})));

	// create_body_from_3d_file(world, "resources/models/watermill.obj", Eigen::Vector3f(0, 2, 0));
	// Eigen::Quaternionf rotation(Eigen::AngleAxisf(PI / 4.7, -Eigen::Vector3f::UnitZ()));
	// create_triangle_body(world, 1.5f, Eigen::Vector3f(-3, 2, 0), rotation);
}
// body system work
///////////////////