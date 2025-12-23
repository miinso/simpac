#pragma once

#include "simplicial.h"
#include <unordered_map>

namespace simplicial {

// ============================================================================
// Hash Keys for Deduplication During Topology Building
// ============================================================================

// Edge key: stores vertices in sorted order for deduplication
struct EdgeKey {
    int v0, v1;

    EdgeKey(int a, int b) : v0(std::min(a, b)), v1(std::max(a, b)) {}

    bool operator==(const EdgeKey& o) const {
        return v0 == o.v0 && v1 == o.v1;
    }
};

struct EdgeKeyHash {
    size_t operator()(const EdgeKey& e) const {
        return std::hash<int>()(e.v0) ^ (std::hash<int>()(e.v1) << 16);
    }
};

// Triangle key: stores vertices in sorted order for deduplication
struct TriKey {
    int v0, v1, v2;

    TriKey(int a, int b, int c) {
        // Sort three values
        if (a > b) std::swap(a, b);
        if (b > c) std::swap(b, c);
        if (a > b) std::swap(a, b);
        v0 = a;
        v1 = b;
        v2 = c;
    }

    bool operator==(const TriKey& o) const {
        return v0 == o.v0 && v1 == o.v1 && v2 == o.v2;
    }
};

struct TriKeyHash {
    size_t operator()(const TriKey& t) const {
        size_t h = std::hash<int>()(t.v0);
        h ^= std::hash<int>()(t.v1) << 10;
        h ^= std::hash<int>()(t.v2) << 20;
        return h;
    }
};

// ============================================================================
// SimplicialTopology - K=(V,E,F,T) Connectivity
// ============================================================================

struct SimplicialTopology {
    // Number of vertices (positions stored in MechanicalState)
    int num_vertices = 0;

    // Primary element storage
    std::vector<Vector2i> edges;       // E: edge vertex pairs
    std::vector<Vector3i> triangles;   // F: triangle vertex triples
    std::vector<Vector4i> tetrahedra;  // T: tet vertex quads

    // Adjacency: vertex -> elements
    std::vector<std::vector<int>> vert_to_edges;
    std::vector<std::vector<int>> vert_to_tris;
    std::vector<std::vector<int>> vert_to_tets;

    // Adjacency: edge -> higher elements
    std::vector<std::vector<int>> edge_to_tris;
    std::vector<std::vector<int>> edge_to_tets;

    // Adjacency: triangle -> tetrahedra
    std::vector<std::vector<int>> tri_to_tets;

    // Surface info (boundary faces for tet meshes)
    std::vector<int> surface_triangles;        // Indices into triangles[]
    std::vector<Vector3i> surface_triangles_oriented;  // Correctly wound surface tris
    bool is_surface_only = true;

    // ========================================================================
    // Query Methods
    // ========================================================================

    int numVertices() const { return num_vertices; }
    int numEdges() const { return static_cast<int>(edges.size()); }
    int numTriangles() const { return static_cast<int>(triangles.size()); }
    int numTetrahedra() const { return static_cast<int>(tetrahedra.size()); }
    bool hasTets() const { return !tetrahedra.empty(); }

    // Find edge index from vertex pair, returns -1 if not found
    int findEdge(int v0, int v1) const;

    // Find triangle index from vertex triple, returns -1 if not found
    int findTriangle(int v0, int v1, int v2) const;

    // ========================================================================
    // Initialization Methods
    // ========================================================================

    // Clear all data
    void clear();

    // Initialize from triangle mesh (surface only)
    void initFromTriangles(int numVerts, const std::vector<Vector3i>& tris);

    // Initialize from tet mesh (volumetric)
    void initFromTetrahedra(int numVerts, const std::vector<Vector4i>& tets);

    // Build all adjacency structures (called automatically by init methods)
    void buildTopology();

    // Extract surface triangles from tet mesh (boundary faces)
    void extractSurface();

private:
    // Edge map for fast lookup during building and queries
    std::unordered_map<EdgeKey, int, EdgeKeyHash> edge_map_;
    std::unordered_map<TriKey, int, TriKeyHash> tri_map_;

    // Internal helpers
    void buildEdgesFromTriangles();
    void buildEdgesFromTetrahedra();
    void buildTrianglesFromTetrahedra();
    void buildVertexAdjacencies();
    void buildEdgeAdjacencies();
    void buildTriangleAdjacencies();
};

} // namespace simplicial
