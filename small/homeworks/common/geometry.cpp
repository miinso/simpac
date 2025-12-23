#include "common/geometry.h"

void init_body_system(flecs::world& world) {
    world.component<body>();
    world.component<trimesh>();
    world.component<aabb>();
}

flecs::entity create_body_from_mesh(flecs::world& world, const Mesh& mesh, const Eigen::Vector3f& position) {
    trimesh t;
    t.from_raylib(mesh);
    t.init();

    body b;
    b.position = position;
    b.orientation = Eigen::Quaternionf::Identity();
    b.scale = Eigen::Vector3f::Ones();

    aabb box;
    body::update_aabb(b, t, box);

    return world.entity()
        .set<body>(b)
        .set<trimesh>(t)
        .set<aabb>(box);
}

flecs::entity create_plane_body(flecs::world& world,
                                float width,
                                float length,
                                const Eigen::Vector3f& position,
                                const Eigen::Vector3f& normal) {
    Mesh mesh = GenMeshPlane(width, length, 1, 1);
    auto entity = create_body_from_mesh(world, mesh, position);

    // Set orientation based on normal
    Eigen::Vector3f up = Eigen::Vector3f::UnitY();
    if (std::abs(normal.dot(up)) < 0.999f) {
        Eigen::Vector3f axis = up.cross(normal).normalized();
        float angle = std::acos(up.dot(normal));
        entity.get_mut<body>().orientation = Eigen::Quaternionf(Eigen::AngleAxisf(angle, axis));
    }

    return entity;
}

flecs::entity create_body_from_3d_file(flecs::world& world,
                                       const char* filename,
                                       const Eigen::Vector3f& position,
                                       const Eigen::Quaternionf& orientation) {
    Model model = LoadModel(filename);
    if (model.meshCount == 0) {
        return flecs::entity();
    }

    trimesh t;
    t.from_raylib(model.meshes[0]);
    t.init();
    t.model = model;

    body b;
    b.position = position;
    b.orientation = orientation;
    b.scale = Eigen::Vector3f::Ones();

    aabb box;
    body::update_aabb(b, t, box);

    return world.entity()
        .set<body>(b)
        .set<trimesh>(t)
        .set<aabb>(box);
}

flecs::entity create_cube_body(flecs::world& world,
                               float width,
                               float height,
                               float depth,
                               const Eigen::Vector3f& position,
                               const Eigen::Quaternionf& orientation) {
    Mesh mesh = GenMeshCube(width, height, depth);
    auto entity = create_body_from_mesh(world, mesh, position);
    entity.get_mut<body>().orientation = orientation;
    return entity;
}

flecs::entity create_sphere_body(flecs::world& world,
                                 float radius,
                                 const Eigen::Vector3f& position,
                                 const Eigen::Quaternionf& orientation) {
    Mesh mesh = GenMeshSphere(radius, 16, 16);
    auto entity = create_body_from_mesh(world, mesh, position);
    entity.get_mut<body>().orientation = orientation;
    return entity;
}

flecs::entity create_poly_body(flecs::world& world,
                               int sides,
                               float radius,
                               const Eigen::Vector3f& position,
                               const Eigen::Quaternionf& orientation) {
    Mesh mesh = GenMeshPoly(sides, radius);
    auto entity = create_body_from_mesh(world, mesh, position);
    entity.get_mut<body>().orientation = orientation;
    return entity;
}

flecs::entity create_triangle_body(flecs::world& world,
                                   float size,
                                   const Eigen::Vector3f& position,
                                   const Eigen::Quaternionf& orientation) {
    Mesh mesh = GenMeshPoly(3, size);
    auto entity = create_body_from_mesh(world, mesh, position);
    entity.get_mut<body>().orientation = orientation;
    return entity;
}

void install_bodies(flecs::world& world) {
    init_body_system(world);

    // Create some default bodies for the scene
    create_plane_body(world, 20.0f, 20.0f, Eigen::Vector3f(0, 0, 0), Eigen::Vector3f::UnitY());
    create_cube_body(world, 1.0f, 1.0f, 1.0f, Eigen::Vector3f(2, 0.5f, 0));
    create_sphere_body(world, 0.5f, Eigen::Vector3f(-2, 0.5f, 0));
}
