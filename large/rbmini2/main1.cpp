#include "graphics.h"
#include <iostream>

#include "core/components.hpp"
#include "core/types.hpp"
#include "systems/dynamics.hpp"
#include "systems/geometry.hpp"
#include "util/logging.hpp"

using namespace phys;
using namespace phys::components;

using namespace Eigen;

int main() {
    flecs::world ecs;

    graphics::init(ecs);
    graphics::init_window(800, 800, "Physics Simulator");

    systems::register_geometry_systems(ecs);
    systems::register_dynamics_systems(ecs);

    MeshFilter filter;
    Mesh m = GenMeshCube(1, 1, 1);
    filter.init_mesh(m);

    // particle (incomplete - Position0 not defined)
    // auto particle =
    //     ecs.entity()
    //         .add<Position>()
    //         .add<Position0>()
    //         .add<LinearVelocity>();

    // rigidbody
    auto e1 =
        ecs.entity()
            .add<phys::components::Dynamic>() // dynamic body
            .add<Position>()
            .add<Orientation>()
            .set<Scale>({Vector3r::Identity()})
            .set<LinearVelocity>({Vector3r(0.0f, 10.0f, 0.0f)})
            .set<AngularVelocity>({Vector3r(3.0f, 0.0f, 0.0f)})
            .add<LinearForce>()
            .add<AngularForce>()
            .set<Mass>({1.0f}) // no braces bc it's just a float
            .add<InverseMass>()
            .set<LocalInertia>({Matrix3r::Identity()})
            .set<WorldInertia>({Matrix3r::Identity()})
            .set<WorldInverseInertia>({Matrix3r::Identity()})
            .set<MeshFilter>(filter)
            .add<MeshRenderer>();

    auto e2 =
        ecs.entity()
            .add<phys::components::Dynamic>() // dynamic body
            .set<Position>({Vector3r(3,0,0)})
            .add<Orientation>()
            .set<Scale>({Vector3r::Identity()})
            .set<LinearVelocity>({Vector3r(0.0f, 10.0f, 0.0f)})
            .set<AngularVelocity>({Vector3r(3.0f, 0.0f, 100.0f)})
            .add<LinearForce>()
            .add<AngularForce>()
            .set<Mass>({1.0f}) // no braces bc it's just a float
            .add<InverseMass>()
            .set<LocalInertia>({Matrix3r::Identity()})
            .set<WorldInertia>({Matrix3r::Identity()})
            .set<WorldInverseInertia>({Matrix3r::Identity()})
            .set<MeshFilter>(filter)
            .add<MeshRenderer>();

    ecs.system<MeshRenderer>().kind(graphics::phase_on_render).each([&](MeshRenderer& renderer) {
        renderer.draw();
        renderer.draw_wireframe();
    });

    ecs.system<MeshFilter>().kind(graphics::phase_on_render).each([&](MeshFilter& mesh) {
        auto& v = mesh.local_vertices.getVertices();
        auto& v2 = mesh.world_vertices.getVertices();
        for (int i = 0; i < mesh.world_vertices.size(); ++i) {
            DrawSphere({v[i].x(), v[i].y(), v[i].z()}, 0.05f, BLUE);
            DrawSphere({v2[i].x(), v2[i].y(), v2[i].z()}, 0.05f, GREEN);
        }
    });

    graphics::run_loop();

    std::cout << "Simulation ended." << std::endl;
    return 0;
}
