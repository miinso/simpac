#pragma once

#include "Eigen/Dense"
#include "flecs.h"
#include "raylib.h"
#include <vector>

struct aabb {
	Eigen::Vector3f min;
	Eigen::Vector3f max;

	aabb()
		: min(Eigen::Vector3f::Zero())
		, max(Eigen::Vector3f::Zero()) { }
	aabb(const Eigen::Vector3f& min, const Eigen::Vector3f& max)
		: min(min)
		, max(max) { }

	aabb& operator=(const aabb& other) {
		min = other.min;
		max = other.max;
		return *this;
	}

	static bool point_in_aabb(const aabb& a, const Eigen::Vector3f& p) {
		return (p.x() >= a.min.x() && p.x() <= a.max.x() && p.y() >= a.min.y() &&
				p.y() <= a.max.y() && p.z() >= a.min.z() && p.z() <= a.max.z());
	}

	static void corner_point(const aabb& a, int i, Eigen::Vector3f& p) {
		p.x() = (i & 1) ? a.max.x() : a.min.x();
		p.y() = (i & 2) ? a.max.y() : a.min.y();
		p.z() = (i & 4) ? a.max.z() : a.min.z();
	}

	static bool intersection(const aabb& a1, const aabb& a2) {
		return (a1.min.x() <= a2.max.x() && a1.max.x() >= a2.min.x() && a1.min.y() <= a2.max.y() &&
				a1.max.y() >= a2.min.y() && a1.min.z() <= a2.max.z() && a1.max.z() >= a2.min.z());
	}
};

struct trimesh {
	std::vector<Eigen::Vector3f> vertices;
	std::vector<int> edge_ids;
	std::vector<int> tri_ids;
	Model model; // raylib Model for rendering

	void init() {
		// Initialize center of mass
		Eigen::Vector3f com = Eigen::Vector3f::Zero();
		for (const auto& v : vertices) {
			com += v;
		}
		com /= vertices.size();
		// Center the mesh at the origin
		for (auto& v : vertices) {
			v -= com;
		}
	}

	void from_raylib(const Mesh& mesh) {
		vertices.clear();
		edge_ids.clear();
		tri_ids.clear();

		for (int i = 0; i < mesh.vertexCount; ++i) {
			vertices.emplace_back(
				mesh.vertices[i * 3], mesh.vertices[i * 3 + 1], mesh.vertices[i * 3 + 2]);
		}

		if (mesh.indices != nullptr) {
			for (int i = 0; i < mesh.triangleCount * 3; i += 3) {
				int v1 = mesh.indices[i];
				int v2 = mesh.indices[i + 1];
				int v3 = mesh.indices[i + 2];
				tri_ids.push_back(v1);
				tri_ids.push_back(v2);
				tri_ids.push_back(v3);
				edge_ids.push_back(v1);
				edge_ids.push_back(v2);
				edge_ids.push_back(v2);
				edge_ids.push_back(v3);
				edge_ids.push_back(v3);
				edge_ids.push_back(v1);
			}
		} else {
			// If there are no indices, assume the vertices are already arranged in triangles
			for (int i = 0; i < mesh.vertexCount; i += 3) {
				tri_ids.push_back(i);
				tri_ids.push_back(i + 1);
				tri_ids.push_back(i + 2);
				edge_ids.push_back(i);
				edge_ids.push_back(i + 1);
				edge_ids.push_back(i + 1);
				edge_ids.push_back(i + 2);
				edge_ids.push_back(i + 2);
				edge_ids.push_back(i);
			}
		}

		Image checked = GenImageChecked(6, 6, 1, 1, LIGHTGRAY, DARKGRAY);
		auto texture = LoadTextureFromImage(checked);
		UnloadImage(checked);

		model = LoadModelFromMesh(mesh);
		model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;
	}

	int num_vertices() const {
		return vertices.size();
	}
	int num_edges() const {
		return edge_ids.size() / 2;
	}
	int num_tris() const {
		return tri_ids.size() / 3;
	}

	void compute_bounding_sphere_for_each_face() {
		// wip
	}
};

struct body {
	Eigen::Vector3f position;
	Eigen::Quaternionf orientation;
	Eigen::Vector3f scale;

	body()
		: position(Eigen::Vector3f::Zero())
		, orientation(Eigen::Quaternionf::Identity())
		, scale(Eigen::Vector3f::Ones()) { }

	static void update_aabb(const body& b, const trimesh& t, aabb& box) {
		box.min = Eigen::Vector3f::Constant(std::numeric_limits<float>::max());
		box.max = Eigen::Vector3f::Constant(std::numeric_limits<float>::lowest());
		for (const auto& v : t.vertices) {
			Eigen::Vector3f transformed = b.orientation * (b.scale.cwiseProduct(v)) + b.position;
			box.min = box.min.cwiseMin(transformed);
			box.max = box.max.cwiseMax(transformed);
		}
	}
};

void init_body_system(flecs::world& world);
flecs::entity
create_body_from_mesh(flecs::world& world, const Mesh& mesh, const Eigen::Vector3f& position);
flecs::entity create_plane_body(flecs::world& world,
								float width,
								float length,
								const Eigen::Vector3f& position,
								const Eigen::Vector3f& normal);
flecs::entity
create_body_from_3d_file(flecs::world& world,
						 const char* filename,
						 const Eigen::Vector3f& position,
						 const Eigen::Quaternionf& orientation = Eigen::Quaternionf::Identity());
flecs::entity create_cube_body(
	flecs::world& world,
	float width,
	float height,
	float depth,
	const Eigen::Vector3f& position,
	const Eigen::Quaternionf& orientation = Eigen::Quaternionf::Identity());
flecs::entity
create_sphere_body(flecs::world& world,
				   float radius,
				   const Eigen::Vector3f& position,
				   const Eigen::Quaternionf& orientation = Eigen::Quaternionf::Identity());
flecs::entity
create_poly_body(flecs::world& world,
				 int sides,
				 float radius,
				 const Eigen::Vector3f& position,
				 const Eigen::Quaternionf& orientation = Eigen::Quaternionf::Identity());

flecs::entity
create_triangle_body(flecs::world& world,
					 float size,
					 const Eigen::Vector3f& position,
					 const Eigen::Quaternionf& orientation = Eigen::Quaternionf::Identity());

void install_bodies(flecs::world& world);

// Utility functions
Vector3 e2r(const Eigen::Vector3f& v);
Eigen::Vector3f r2e(const Vector3& v);
Matrix e2r_matrix(const Eigen::Matrix4f& m);