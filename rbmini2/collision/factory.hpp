// collision/factory.hpp

#pragma once
#include "collision/components.hpp"
// #include "collision/types.hpp"
// #include "cubic_lagrange_discrete_grid.hpp" // from discregrid
// #include "geometry/TriangleMeshDistance.h" // from discregrid
#include "sdf/types.hpp"

namespace phys {
    namespace collision {

        using phys::sdf::GridSDF;
        // using phys::sdf::SDFData;
        using phys::sdf::SDFType;

        // inline GridSDF create_grid_sdf(const std::vector<Vector3r>& vertices,
        //                                const std::vector<unsigned int>& indices,
        //                                const Vector3i& resolution) {

        //     // auto data = new GridSDF();
        //     GridSDF data;

        //     std::vector<double> vertices_double;
        //     vertices_double.reserve(vertices.size() * 3);
        //     for (const auto& v : vertices) {
        //         vertices_double.push_back(v.x());
        //         vertices_double.push_back(v.y());
        //         vertices_double.push_back(v.z());
        //     }

        //     Discregrid::TriangleMesh mesh(
        //         vertices_double.data(), indices.data(), vertices.size(), indices.size() / 3);

        //     Eigen::AlignedBox3d domain;
        //     for (const auto& v : vertices) {
        //         domain.extend(v.cast<double>());
        //     }
        //     domain.max() += 0.1 * Eigen::Vector3d::Ones();
        //     domain.min() -= 0.1 * Eigen::Vector3d::Ones();

        //     data.grid = std::make_unique<Discregrid::CubicLagrangeDiscreteGrid>(
        //         domain,
        //         std::array<unsigned int, 3>({(unsigned int)resolution.x(),
        //                                      (unsigned int)resolution.y(),
        //                                      (unsigned int)resolution.z()}));

        //     Discregrid::TriangleMeshDistance md(mesh);
        //     auto func = [&md](Eigen::Vector3d const& xi) {
        //         return md.signed_distance(xi).distance;
        //     };
        //     data.grid->addFunction(func, true);

        //     data.origin = domain.min().cast<Real>();
        //     data.cell_size =
        //         (domain.max() - domain.min()).cast<Real>().cwiseQuotient(resolution.cast<Real>());
        //     data.resolution = resolution;

        //     return data;
        // }

        inline components::SDFCollider create_sphere_collider(Real radius) {
            components::SDFCollider collider;
            collider.type = SDFType::Sphere;
            collider.data.sphere = {radius};
            return collider;
        }

        inline components::SDFCollider create_box_collider(const Vector3r& half_extents) {
            components::SDFCollider collider;
            collider.type = SDFType::Box;
            collider.data.box = {half_extents};
            return collider;
        }

        inline components::SDFCollider create_capsule_collider(Real radius, Real height) {
            components::SDFCollider collider;
            collider.type = SDFType::Capsule;
            collider.data.capsule = {radius, height};
            return collider;
        }

        inline components::SDFCollider
        create_mesh_collider(const std::vector<Vector3r>& vertices,
                             const std::vector<unsigned int>& indices,
                             const Vector3i& resolution = Vector3i(10, 10, 10)) {

            // assert for empty verts maybe?
            components::SDFCollider collider;
            collider.type = SDFType::Grid;
            collider.data.grid = create_grid_sdf(vertices, indices, resolution);
            return std::move(collider);
        }
    } // namespace collision
} // namespace phys