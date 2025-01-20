#include <Eigen/Dense>
#include <flecs.h>
#include <iostream>
#include "graphics.h"
#include <memory>
// sdf
#include "cubic_lagrange_discrete_grid.hpp"
#include "geometry/TriangleMeshDistance.h"
#include "mesh/triangle_mesh.hpp"
#include "IndexedFaceMesh.h"
#include "ParticleData.h"
#include "VolumeIntegration.h"

struct Position {
    double x, y;
};

struct Velocity {
    double x, y;
};

struct Collider {
    // Discregrid::CubicLagrangeDiscreteGrid sdf;
    std::unique_ptr<Discregrid::CubicLagrangeDiscreteGrid> sdf;
    Vector3r scale;
    Real invertSDF; // 1 for normal objects, -1 to invert inside/outside

    // distance query with tolerance
    double distance(const Eigen::Vector3d& x, const Real tolerance) const {
        const Eigen::Vector3d scaled_x = x.cwiseProduct(scale.template cast<double>().cwiseInverse());
        const double dist = sdf->interpolate(0, scaled_x);
        if (dist == std::numeric_limits<double>::max())
            return dist;
        return invertSDF * scale[0] * dist - tolerance;
    }

    // collision test with contact point and normal
    bool collision_test(const Vector3r& x,
                        const Real tolerance,
                        Vector3r& cp,
                        Vector3r& n,
                        Real& dist,
                        const Real maxDist = 0.0) const {
        const Vector3r scaled_x = x.cwiseProduct(scale.cwiseInverse());

        Eigen::Vector3d normal;
        double d = sdf->interpolate(0, scaled_x.template cast<double>(), &normal);
        if (d == std::numeric_limits<Real>::max())
            return false;
        dist = static_cast<Real>(invertSDF * d - tolerance);

        normal = invertSDF * normal;
        if (dist < maxDist) {
            normal.normalize();
            n = normal.template cast<Real>();

            cp = (scaled_x - dist * n);
            cp = cp.cwiseProduct(scale);

            // assert(false);
            return true;
        }
        return false;
    }
};

int main() {
    // Print a message
    std::cout << "Hello, Eigen!" << std::endl;

    // Define a 2x2 matrix
    Eigen::Matrix2d mat;
    mat << 1, 2, 3, 4;

    // Print the matrix
    std::cout << "Here is the matrix mat:\n" << mat << std::endl;

    ////
    flecs::world ecs;

    flecs::system s = ecs.system<Position, const Velocity>().each(
        [](flecs::entity e, Position& p, const Velocity& v) {
            p.x += v.x;
            p.y += v.y;
            std::cout << e.name() << ": {" << p.x << ", " << p.y << "}\n";
        });

    ecs.entity("e1").set<Position>({10, 20}).set<Velocity>({1, 2});

    graphics::init(ecs);
    graphics::init_window(800, 800, "Physics Simulator");

    ecs.system("you can do it").kind(graphics::PostRender).run([](flecs::iter& it) {
        rl::DrawText("You Can DO iT!!!", 200, 200, 40, rl::DARKGREEN);
    });

    graphics::run_main_loop([]() { });

    std::cout << "Simulation ended." << std::endl;
    return 0;
}