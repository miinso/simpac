#include <Eigen/Dense>
#include <flecs.h>

#include "VolumeIntegration.h"
// sdf
#include "cubic_lagrange_discrete_grid.hpp"
#include "geometry/TriangleMeshDistance.h"

#include "components.hpp"

// collider component with sdf
struct Collider {
    // Discregrid::CubicLagrangeDiscreteGrid sdf;
    std::unique_ptr<Discregrid::CubicLagrangeDiscreteGrid> sdf;
    Vector3r scale;
    Real invertSDF; // 1 for normal objects, -1 to invert inside/outside

    // distance query with tolerance
    double distance(const Eigen::Vector3d& x, const Real tolerance) const {
        const Eigen::Vector3d scaled_x =
            x.cwiseProduct(scale.template cast<double>().cwiseInverse());
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

Discregrid::CubicLagrangeDiscreteGrid
generate_sdf(const std::vector<Vector3r>& vertices,
             const std::vector<unsigned int>& faces,
             const Eigen::Matrix<unsigned int, 3, 1>& resolution,
             const std::pair<float, float>& extension = std::make_pair(0.0f, 0.0f)) {
    const unsigned int nFaces = faces.size() / 3;
#ifdef USE_DOUBLE
    Discregrid::TriangleMesh sdfMesh((vertices[0]).data(), faces.data(), vertices.size(), nFaces);
#else
    // if type is float, copy vector to double vector
    std::vector<double> doubleVec;
    doubleVec.resize(3 * vertices.size());
    for (unsigned int i = 0; i < vertices.size(); i++)
        for (unsigned int j = 0; j < 3; j++)
            doubleVec[3 * i + j] = vertices[i][j];
    Discregrid::TriangleMesh sdfMesh(&doubleVec[0], faces.data(), vertices.size(), nFaces);
#endif
    Discregrid::TriangleMeshDistance md(sdfMesh);
    Eigen::AlignedBox3d domain;
    for (auto const& x : sdfMesh.vertices()) {
        domain.extend(x);
    }
    domain.max() += 0.1 * Eigen::Vector3d::Ones();
    domain.min() -= 0.1 * Eigen::Vector3d::Ones();

    domain.min() -= extension.first * Eigen::Vector3d::Ones();
    domain.max() += extension.second * Eigen::Vector3d::Ones();

    std::cout << "Set SDF resolution: " << resolution[0] << ", " << resolution[1] << ", "
              << resolution[2] << std::endl;

    Discregrid::CubicLagrangeDiscreteGrid sdf(
        domain, std::array<unsigned int, 3>({resolution[0], resolution[1], resolution[2]}));

    auto func = Discregrid::DiscreteGrid::ContinuousFunction{};

    func = [&md](Eigen::Vector3d const& xi) { return md.signed_distance(xi).distance; };

    std::cout << "Generate SDF\n";

    sdf.addFunction(func, true);
    return sdf;
}

// Initialize collider for an entity
void init_collider(flecs::entity entity,
                   const std::vector<Vector3r>& vertices,
                   const std::vector<unsigned int>& indices) {
    if (!entity.has<Collider>()) {
        entity.add<Collider>();
    }
    auto collider = entity.get_mut<Collider>();

    // initialize sdf using the vertex/face data
    Eigen::Matrix<unsigned int, 3, 1> resolution{20, 20, 20};
    auto sdf = generate_sdf(vertices, indices, resolution);
    collider->sdf = std::make_unique<Discregrid::CubicLagrangeDiscreteGrid>(std::move(sdf));
    collider->scale = Eigen::Vector3f::Ones();
    collider->invertSDF = 1.0f;
}

void init_collider_from_rlmesh(flecs::entity entity, const rl::Mesh& rlMesh) {
    if (!entity.has<Collider>()) {
        entity.add<Collider>();
    }

    auto collider = entity.get_mut<Collider>();

    Eigen::Matrix<unsigned int, 3, 1> resolution{20, 20, 20};

    std::vector<Vector3r> vertices(rlMesh.vertexCount);
    for (int i = 0; i < rlMesh.vertexCount; i++) {
        vertices[i] = Vector3r(
            rlMesh.vertices[i * 3], rlMesh.vertices[i * 3 + 1], rlMesh.vertices[i * 3 + 2]);
    }

    std::vector<unsigned int> indices(rlMesh.triangleCount * 3);
    memcpy(indices.data(), rlMesh.indices, sizeof(unsigned int) * rlMesh.triangleCount * 3);

    auto sdf = generate_sdf(vertices, indices, resolution);
    collider->sdf = std::make_unique<Discregrid::CubicLagrangeDiscreteGrid>(std::move(sdf));
    collider->scale = Eigen::Vector3f::Ones();
    collider->invertSDF = 1.0f;
}

void init_sdfcollider_from_geometry(flecs::entity entity) {
    if (!entity.has<Collider>()) {
        entity.add<Collider>();
    }

    if (!entity.has<Collider>() || !entity.has<Geometry>()) {
        return;
    }
    auto collider = entity.get_mut<Collider>();
    auto geom = entity.get<Geometry>();

    Eigen::Matrix<unsigned int, 3, 1> resolution{20, 20, 20};

    const auto& vertices = geom->verts0.getVertices();
    const auto& indices = geom->mesh.getFaces();

    auto sdf = generate_sdf(vertices, indices, resolution);
    collider->sdf = std::make_unique<Discregrid::CubicLagrangeDiscreteGrid>(std::move(sdf));
    collider->scale = Eigen::Vector3f::Ones();
    collider->invertSDF = 1.0f;
}