#include "topology.h"

namespace simplicial {

// ============================================================================
// Query Methods
// ============================================================================

int SimplicialTopology::findEdge(int v0, int v1) const {
    auto it = edge_map_.find(EdgeKey(v0, v1));
    return (it != edge_map_.end()) ? it->second : -1;
}

int SimplicialTopology::findTriangle(int v0, int v1, int v2) const {
    auto it = tri_map_.find(TriKey(v0, v1, v2));
    return (it != tri_map_.end()) ? it->second : -1;
}

// ============================================================================
// Initialization Methods
// ============================================================================

void SimplicialTopology::clear() {
    num_vertices = 0;
    edges.clear();
    triangles.clear();
    tetrahedra.clear();
    vert_to_edges.clear();
    vert_to_tris.clear();
    vert_to_tets.clear();
    edge_to_tris.clear();
    edge_to_tets.clear();
    tri_to_tets.clear();
    surface_triangles.clear();
    edge_map_.clear();
    tri_map_.clear();
    is_surface_only = true;
}

void SimplicialTopology::initFromTriangles(int numVerts, const std::vector<Vector3i>& tris) {
    clear();
    num_vertices = numVerts;
    triangles = tris;
    is_surface_only = true;

    // Build triangle map
    for (int i = 0; i < static_cast<int>(triangles.size()); ++i) {
        const auto& t = triangles[i];
        tri_map_[TriKey(t[0], t[1], t[2])] = i;
    }

    buildTopology();

    // All triangles are surface for surface-only mesh
    surface_triangles.resize(triangles.size());
    for (int i = 0; i < static_cast<int>(triangles.size()); ++i) {
        surface_triangles[i] = i;
    }
}

void SimplicialTopology::initFromTetrahedra(int numVerts, const std::vector<Vector4i>& tets) {
    clear();
    num_vertices = numVerts;
    tetrahedra = tets;
    is_surface_only = false;

    buildTopology();
    extractSurface();
}

void SimplicialTopology::buildTopology() {
    if (is_surface_only) {
        buildEdgesFromTriangles();
    } else {
        buildTrianglesFromTetrahedra();
        buildEdgesFromTetrahedra();
    }

    buildVertexAdjacencies();
    buildEdgeAdjacencies();

    if (!is_surface_only) {
        buildTriangleAdjacencies();
    }
}

// ============================================================================
// Edge Building
// ============================================================================

void SimplicialTopology::buildEdgesFromTriangles() {
    edge_map_.clear();
    edges.clear();

    // 3 edges per triangle: (0,1), (1,2), (2,0)
    constexpr int TRI_EDGES[3][2] = {{0, 1}, {1, 2}, {2, 0}};

    for (const auto& tri : triangles) {
        for (const auto& pair : TRI_EDGES) {
            EdgeKey key(tri[pair[0]], tri[pair[1]]);
            if (edge_map_.find(key) == edge_map_.end()) {
                edge_map_[key] = static_cast<int>(edges.size());
                edges.push_back(Vector2i(key.v0, key.v1));
            }
        }
    }
}

void SimplicialTopology::buildEdgesFromTetrahedra() {
    edge_map_.clear();
    edges.clear();

    // 6 edges per tetrahedron
    constexpr int TET_EDGES[6][2] = {
        {0, 1}, {0, 2}, {0, 3}, {1, 2}, {1, 3}, {2, 3}
    };

    for (const auto& tet : tetrahedra) {
        for (const auto& pair : TET_EDGES) {
            EdgeKey key(tet[pair[0]], tet[pair[1]]);
            if (edge_map_.find(key) == edge_map_.end()) {
                edge_map_[key] = static_cast<int>(edges.size());
                edges.push_back(Vector2i(key.v0, key.v1));
            }
        }
    }
}

// ============================================================================
// Triangle Building (from tetrahedra)
// ============================================================================

void SimplicialTopology::buildTrianglesFromTetrahedra() {
    tri_map_.clear();
    triangles.clear();

    // 4 faces per tetrahedron (opposite each vertex)
    // Face opposite to vertex i contains the other 3 vertices
    constexpr int TET_FACES[4][3] = {
        {1, 2, 3},  // opposite vertex 0
        {0, 2, 3},  // opposite vertex 1
        {0, 1, 3},  // opposite vertex 2
        {0, 1, 2}   // opposite vertex 3
    };

    for (const auto& tet : tetrahedra) {
        for (const auto& face : TET_FACES) {
            TriKey key(tet[face[0]], tet[face[1]], tet[face[2]]);
            if (tri_map_.find(key) == tri_map_.end()) {
                tri_map_[key] = static_cast<int>(triangles.size());
                triangles.push_back(Vector3i(key.v0, key.v1, key.v2));
            }
        }
    }
}

// ============================================================================
// Adjacency Building
// ============================================================================

void SimplicialTopology::buildVertexAdjacencies() {
    // Vertex -> edges
    vert_to_edges.clear();
    vert_to_edges.resize(num_vertices);
    for (int ei = 0; ei < static_cast<int>(edges.size()); ++ei) {
        const auto& e = edges[ei];
        vert_to_edges[e[0]].push_back(ei);
        vert_to_edges[e[1]].push_back(ei);
    }

    // Vertex -> triangles
    vert_to_tris.clear();
    vert_to_tris.resize(num_vertices);
    for (int ti = 0; ti < static_cast<int>(triangles.size()); ++ti) {
        const auto& t = triangles[ti];
        vert_to_tris[t[0]].push_back(ti);
        vert_to_tris[t[1]].push_back(ti);
        vert_to_tris[t[2]].push_back(ti);
    }

    // Vertex -> tetrahedra
    vert_to_tets.clear();
    vert_to_tets.resize(num_vertices);
    for (int ti = 0; ti < static_cast<int>(tetrahedra.size()); ++ti) {
        const auto& t = tetrahedra[ti];
        vert_to_tets[t[0]].push_back(ti);
        vert_to_tets[t[1]].push_back(ti);
        vert_to_tets[t[2]].push_back(ti);
        vert_to_tets[t[3]].push_back(ti);
    }
}

void SimplicialTopology::buildEdgeAdjacencies() {
    // Edge -> triangles
    edge_to_tris.clear();
    edge_to_tris.resize(edges.size());

    constexpr int TRI_EDGES[3][2] = {{0, 1}, {1, 2}, {2, 0}};

    for (int ti = 0; ti < static_cast<int>(triangles.size()); ++ti) {
        const auto& tri = triangles[ti];
        for (const auto& pair : TRI_EDGES) {
            int ei = findEdge(tri[pair[0]], tri[pair[1]]);
            if (ei >= 0) {
                edge_to_tris[ei].push_back(ti);
            }
        }
    }

    // Edge -> tetrahedra
    edge_to_tets.clear();
    edge_to_tets.resize(edges.size());

    constexpr int TET_EDGES[6][2] = {
        {0, 1}, {0, 2}, {0, 3}, {1, 2}, {1, 3}, {2, 3}
    };

    for (int ti = 0; ti < static_cast<int>(tetrahedra.size()); ++ti) {
        const auto& tet = tetrahedra[ti];
        for (const auto& pair : TET_EDGES) {
            int ei = findEdge(tet[pair[0]], tet[pair[1]]);
            if (ei >= 0) {
                edge_to_tets[ei].push_back(ti);
            }
        }
    }
}

void SimplicialTopology::buildTriangleAdjacencies() {
    // Triangle -> tetrahedra
    tri_to_tets.clear();
    tri_to_tets.resize(triangles.size());

    constexpr int TET_FACES[4][3] = {
        {1, 2, 3}, {0, 2, 3}, {0, 1, 3}, {0, 1, 2}
    };

    for (int ti = 0; ti < static_cast<int>(tetrahedra.size()); ++ti) {
        const auto& tet = tetrahedra[ti];
        for (const auto& face : TET_FACES) {
            int fi = findTriangle(tet[face[0]], tet[face[1]], tet[face[2]]);
            if (fi >= 0) {
                tri_to_tets[fi].push_back(ti);
            }
        }
    }
}

// ============================================================================
// Surface Extraction
// ============================================================================

void SimplicialTopology::extractSurface() {
    surface_triangles.clear();
    surface_triangles_oriented.clear();

    if (tetrahedra.empty()) {
        // All triangles are surface for surface-only mesh
        surface_triangles.resize(triangles.size());
        surface_triangles_oriented = triangles;  // Keep original winding
        for (int i = 0; i < static_cast<int>(triangles.size()); ++i) {
            surface_triangles[i] = i;
        }
        return;
    }

    // For tet meshes: extract boundary faces with correct outward winding
    // Face opposite vertex i has vertices from the other 3, wound to point away from vertex i
    // Standard outward-facing winding for positive-volume tet:
    constexpr int TET_FACES_OUTWARD[4][3] = {
        {1, 3, 2},  // opposite v0: wind so normal points away from v0
        {0, 2, 3},  // opposite v1: wind so normal points away from v1
        {0, 3, 1},  // opposite v2: wind so normal points away from v2
        {0, 1, 2}   // opposite v3: wind so normal points away from v3
    };

    // Count face occurrences and track which tet owns each face
    std::unordered_map<TriKey, int, TriKeyHash> face_count;
    std::unordered_map<TriKey, std::pair<int, int>, TriKeyHash> face_owner;  // tet_idx, face_idx

    for (int ti = 0; ti < static_cast<int>(tetrahedra.size()); ++ti) {
        const auto& tet = tetrahedra[ti];
        for (int fi = 0; fi < 4; ++fi) {
            const auto& face = TET_FACES_OUTWARD[fi];
            TriKey key(tet[face[0]], tet[face[1]], tet[face[2]]);
            face_count[key]++;
            face_owner[key] = {ti, fi};
        }
    }

    // Surface faces appear exactly once
    for (const auto& [key, count] : face_count) {
        if (count == 1) {
            // Find which tet and face this belongs to
            auto [tet_idx, face_idx] = face_owner[key];
            const auto& tet = tetrahedra[tet_idx];
            const auto& face = TET_FACES_OUTWARD[face_idx];

            // Store with correct outward winding
            Vector3i oriented_tri(tet[face[0]], tet[face[1]], tet[face[2]]);
            surface_triangles_oriented.push_back(oriented_tri);

            // Also store index into triangles[] for adjacency queries
            int tri_idx = findTriangle(key.v0, key.v1, key.v2);
            if (tri_idx >= 0) {
                surface_triangles.push_back(tri_idx);
            }
        }
    }
}

} // namespace simplicial
