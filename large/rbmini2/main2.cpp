#include "graphics.h"
#include <iostream>

#include "core/components.hpp"
#include "core/types.hpp"
#include "systems/geometry.hpp"
#include "util/logging.hpp"

using namespace phys;
using namespace phys::components;

using namespace Eigen;

int main() {
    flecs::world ecs;

    graphics::init(ecs);
    graphics::init_window(800, 800, "Physics Simulator");

    WorldInertia inertia;
    inertia.value = Matrix3f::Identity();

    auto entity = ecs.entity().add<Position>().add<Orientation>().add<Scale>();

    systems::register_geometry_systems(ecs);

    auto aa = GenMeshCube(1, 1, 1);

    MeshFilter filter;
    Mesh m = GenMeshCube(1, 1, 1);

    filter.init_mesh(m);

    // UnloadMesh(m);

    auto e1 =
        ecs.entity()
            .add<phys::components::Dynamic>() // dynamic body
            .add<Position>()
            .add<Orientation>()
            .set<Scale>({Vector3r::Identity()})
            .add<LinearVelocity>()
            .add<AngularVelocity>()
            .add<LinearForce>()
            .add<AngularForce>()
            .add<Mass>() // no braces bc it's just a float
            .add<InverseMass>()
            .add<LocalInertia>()
            .add<WorldInertia>()
            .add<WorldInverseInertia>()
            .set<MeshFilter>(filter)
            // .set<WorldMesh>(world_mesh)
            .add<MeshRenderer>();

    // load some mesh
    // auto local_geom = entity.get_mut<LocalGeometry>();

    ecs.system<MeshRenderer>().kind(graphics::OnRender).each([&](MeshRenderer& renderer) {
        renderer.draw();
        renderer.draw_wireframe();
    });

    graphics::run_loop();

    std::cout << "Simulation ended." << std::endl;
    return 0;
}
