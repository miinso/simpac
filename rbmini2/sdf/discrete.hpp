// sdf/discrete.hpp
#pragma once
#include "cubic_lagrange_discrete_grid.hpp"
#include "geometry/TriangleMeshDistance.h" // from discregrid
#include "sdf/components.hpp"

namespace phys {
    namespace sdf {

        inline components::GridSDF*
        generate_grid_sdf(const std::vector<Vector3r>& vertices,
                          const std::vector<unsigned int>& faces,
                          const Vector3i& resolution,
                          const std::pair<float, float>& extension = std::make_pair(0.0f, 0.0f)) {
            const unsigned int nFaces = faces.size() / 3;

            // Convert vertices to double for mesh distance computation
            std::vector<double> doubleVec;
            doubleVec.resize(3 * vertices.size());
            for (unsigned int i = 0; i < vertices.size(); i++)
                for (unsigned int j = 0; j < 3; j++)
                    doubleVec[3 * i + j] = vertices[i][j];

            Discregrid::TriangleMesh sdfMesh(&doubleVec[0], faces.data(), vertices.size(), nFaces);
            Discregrid::TriangleMeshDistance md(sdfMesh);

            // Compute bounds and add margin
            Eigen::AlignedBox3d domain;
            for (auto const& x : sdfMesh.vertices()) {
                domain.extend(x);
            }
            domain.max() += 0.1 * Eigen::Vector3d::Ones();
            domain.min() -= 0.1 * Eigen::Vector3d::Ones();

            // Apply additional extensions if specified
            domain.min() -= extension.first * Eigen::Vector3d::Ones();
            domain.max() += extension.second * Eigen::Vector3d::Ones();

            std::cout << "Set SDF resolution: " << resolution[0] << ", " << resolution[1] << ", "
                      << resolution[2] << std::endl;

            auto* gridSDF = new components::GridSDF();
            gridSDF->grid = new Discregrid::CubicLagrangeDiscreteGrid(
                domain,
                std::array<unsigned int, 3>({static_cast<unsigned int>(resolution[0]),
                                             static_cast<unsigned int>(resolution[1]),
                                             static_cast<unsigned int>(resolution[2])}));

            auto func = [&md](Eigen::Vector3d const& xi) {
                return md.signed_distance(xi).distance;
            };

            std::cout << "Generate SDF\n";
            gridSDF->grid->addFunction(func, true);

            return gridSDF;
        }
    } // namespace sdf
} // namespace phys