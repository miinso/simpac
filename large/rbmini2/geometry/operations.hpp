// geometry/operations.hpp
#pragma once
#include "geometry/components.hpp"
#include "geometry/types.hpp"
#include <cassert>

namespace phys {
    namespace geometry {
        inline void
        compute_bounds(Vector3r& min_bound, Vector3r& max_bound, const PBD::VertexData& vertices) {

            if (vertices.size() == 0) {
                min_bound = max_bound = Vector3r::Zero();
                return;
            }

            min_bound = max_bound = vertices.getPosition(0);

            for (unsigned int i = 1; i < vertices.size(); ++i) {
                const Vector3r& p = vertices.getPosition(i);
                min_bound = min_bound.cwiseMin(p);
                max_bound = max_bound.cwiseMax(p);
            }
        }

        inline void interpolate_vertices(
            PBD::VertexData& result, const PBD::VertexData& a, const PBD::VertexData& b, Real t) {

            assert(a.size() == b.size());
            result.resize(a.size());

            for (unsigned int i = 0; i < a.size(); ++i) {
                Vector3r interpolated = a.getPosition(i) * (1 - t) + b.getPosition(i) * t;
                result.setPosition(i, interpolated);
            }
        }

        inline void weld_vertices(components::MeshFilter& filter, Real threshold = 1e-7) {

            std::vector<unsigned int> remap(filter.get_vertex_count());
            std::vector<Vector3r> unique_positions;
            unsigned int next_index = 0;

            // find unique points
            for (unsigned int i = 0; i < filter.get_vertex_count(); ++i) {
                const Vector3r& p = filter.local_vertices.getPosition(i);
                bool found = false;

                // check if a duplicate
                for (unsigned int j = 0; j < unique_positions.size(); ++j) {
                    if ((p - unique_positions[j]).squaredNorm() < threshold) {
                        remap[i] = j;
                        found = true;
                        break;
                    }
                }

                if (!found) {
                    remap[i] = next_index++;
                    unique_positions.push_back(p);
                }
            }

            // remesh
            PBD::VertexData new_vertices;
            new_vertices.resize(unique_positions.size());
            for (unsigned int i = 0; i < unique_positions.size(); ++i) {
                new_vertices.setPosition(i, unique_positions[i]);
            }

            // new indices
            auto& faces = filter.mesh.getFaces();
            for (unsigned int i = 0; i < faces.size(); ++i) {
                faces[i] = remap[faces[i]];
            }

            // new verts
            filter.local_vertices = new_vertices;

            // new topology
            filter.mesh.buildNeighbors();
        }

    } // namespace geometry
} // namespace phys