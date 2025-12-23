// topology_test.cpp
// Tests for SimplicialTopology - K=(V,E,F,T) simplicial complex
//
// Usage examples embedded in tests demonstrate:
// - Creating topology from triangle meshes
// - Creating topology from tetrahedral meshes
// - Querying adjacencies (vertex->edge, vertex->tri, edge->tri, etc.)
// - Surface extraction from tet meshes

#include "modules/simplicial/simplicial.h"
#include "modules/simplicial/topology.h"
#include <gtest/gtest.h>
#include <cmath>

using namespace simplicial;

// ============================================================================
// Triangle Mesh Tests (Surface-only K=(V,E,F))
// ============================================================================

// Basic: Create topology from a single triangle
TEST(TopologyTriangleTest, SingleTriangle) {
    // A single triangle has 3 vertices, 3 edges, 1 face
    //
    //     2
    //    / \
    //   /   \
    //  0-----1

    std::vector<Vector3i> tris = { Vector3i(0, 1, 2) };

    SimplicialTopology topo;
    topo.initFromTriangles(3, tris);

    EXPECT_EQ(topo.numVertices(), 3);
    EXPECT_EQ(topo.numTriangles(), 1);
    EXPECT_EQ(topo.numEdges(), 3);  // edges: (0,1), (1,2), (2,0)
    EXPECT_EQ(topo.numTetrahedra(), 0);
    EXPECT_TRUE(topo.is_surface_only);
}

// Two triangles sharing an edge
TEST(TopologyTriangleTest, TwoTrianglesSharedEdge) {
    // Two triangles share edge (1,2)
    //
    //     2
    //    /|\
    //   / | \
    //  0--+--3
    //   \ | /
    //    \|/
    //     1

    std::vector<Vector3i> tris = {
        Vector3i(0, 1, 2),
        Vector3i(1, 3, 2)
    };

    SimplicialTopology topo;
    topo.initFromTriangles(4, tris);

    EXPECT_EQ(topo.numVertices(), 4);
    EXPECT_EQ(topo.numTriangles(), 2);
    // 3 edges from first tri + 2 new edges from second tri = 5
    // (edge (1,2) is shared)
    EXPECT_EQ(topo.numEdges(), 5);
}

// Quad split into 2 triangles
TEST(TopologyTriangleTest, QuadMesh) {
    // A quad split into 2 triangles
    //
    //  3---2
    //  |\ /|
    //  | X |  (diagonal from 0 to 2)
    //  |/ \|
    //  0---1

    std::vector<Vector3i> tris = {
        Vector3i(0, 1, 2),
        Vector3i(0, 2, 3)
    };

    SimplicialTopology topo;
    topo.initFromTriangles(4, tris);

    EXPECT_EQ(topo.numVertices(), 4);
    EXPECT_EQ(topo.numTriangles(), 2);
    EXPECT_EQ(topo.numEdges(), 5);  // 4 boundary + 1 diagonal
}

// ============================================================================
// Edge Lookup Tests
// ============================================================================

// findEdge returns edge index, order-independent
TEST(TopologyEdgeLookupTest, FindEdgeOrderIndependent) {
    std::vector<Vector3i> tris = { Vector3i(0, 1, 2) };

    SimplicialTopology topo;
    topo.initFromTriangles(3, tris);

    // Find edge (0,1) - should work both ways
    int e01 = topo.findEdge(0, 1);
    int e10 = topo.findEdge(1, 0);

    EXPECT_GE(e01, 0);
    EXPECT_EQ(e01, e10);  // Same edge regardless of order
}

// Non-existent edge returns -1
TEST(TopologyEdgeLookupTest, NonExistentEdge) {
    std::vector<Vector3i> tris = { Vector3i(0, 1, 2) };

    SimplicialTopology topo;
    topo.initFromTriangles(3, tris);

    // Vertex 5 doesn't exist
    int e_bad = topo.findEdge(0, 5);
    EXPECT_EQ(e_bad, -1);
}

// ============================================================================
// Triangle Lookup Tests
// ============================================================================

// findTriangle returns triangle index, order-independent
TEST(TopologyTriangleLookupTest, FindTriangleOrderIndependent) {
    std::vector<Vector3i> tris = { Vector3i(0, 1, 2) };

    SimplicialTopology topo;
    topo.initFromTriangles(3, tris);

    // All permutations should find the same triangle
    int t012 = topo.findTriangle(0, 1, 2);
    int t021 = topo.findTriangle(0, 2, 1);
    int t120 = topo.findTriangle(1, 2, 0);
    int t102 = topo.findTriangle(1, 0, 2);
    int t201 = topo.findTriangle(2, 0, 1);
    int t210 = topo.findTriangle(2, 1, 0);

    EXPECT_GE(t012, 0);
    EXPECT_EQ(t012, t021);
    EXPECT_EQ(t012, t120);
    EXPECT_EQ(t012, t102);
    EXPECT_EQ(t012, t201);
    EXPECT_EQ(t012, t210);
}

// ============================================================================
// Vertex Adjacency Tests
// ============================================================================

// vert_to_edges: which edges touch a vertex
TEST(TopologyAdjacencyTest, VertexToEdges) {
    //     2
    //    / \
    //   /   \
    //  0-----1

    std::vector<Vector3i> tris = { Vector3i(0, 1, 2) };

    SimplicialTopology topo;
    topo.initFromTriangles(3, tris);

    // Each vertex touches 2 edges in a single triangle
    EXPECT_EQ(topo.vert_to_edges[0].size(), 2u);  // edges (0,1) and (0,2)
    EXPECT_EQ(topo.vert_to_edges[1].size(), 2u);  // edges (0,1) and (1,2)
    EXPECT_EQ(topo.vert_to_edges[2].size(), 2u);  // edges (0,2) and (1,2)
}

// vert_to_tris: which triangles contain a vertex
TEST(TopologyAdjacencyTest, VertexToTriangles) {
    std::vector<Vector3i> tris = {
        Vector3i(0, 1, 2),
        Vector3i(1, 3, 2)
    };

    SimplicialTopology topo;
    topo.initFromTriangles(4, tris);

    // Vertex 0: only in triangle 0
    EXPECT_EQ(topo.vert_to_tris[0].size(), 1u);
    EXPECT_EQ(topo.vert_to_tris[0][0], 0);

    // Vertex 1: in both triangles
    EXPECT_EQ(topo.vert_to_tris[1].size(), 2u);

    // Vertex 2: in both triangles
    EXPECT_EQ(topo.vert_to_tris[2].size(), 2u);

    // Vertex 3: only in triangle 1
    EXPECT_EQ(topo.vert_to_tris[3].size(), 1u);
    EXPECT_EQ(topo.vert_to_tris[3][0], 1);
}

// ============================================================================
// Edge to Triangle Adjacency Tests
// ============================================================================

// edge_to_tris: which triangles share an edge
TEST(TopologyAdjacencyTest, EdgeToTriangles) {
    std::vector<Vector3i> tris = {
        Vector3i(0, 1, 2),
        Vector3i(1, 3, 2)
    };

    SimplicialTopology topo;
    topo.initFromTriangles(4, tris);

    // Shared edge (1,2) is adjacent to both triangles
    int e12 = topo.findEdge(1, 2);
    ASSERT_GE(e12, 0);
    EXPECT_EQ(topo.edge_to_tris[e12].size(), 2u);

    // Boundary edge (0,1) is adjacent to only triangle 0
    int e01 = topo.findEdge(0, 1);
    ASSERT_GE(e01, 0);
    EXPECT_EQ(topo.edge_to_tris[e01].size(), 1u);
}

// ============================================================================
// Tetrahedra Mesh Tests (Volumetric K=(V,E,F,T))
// ============================================================================

// Single tetrahedron
TEST(TopologyTetTest, SingleTetrahedron) {
    // A tetrahedron has 4 vertices, 6 edges, 4 faces, 1 tet
    //
    //       3
    //      /|\
    //     / | \
    //    /  |  \
    //   0---|---2
    //    \  |  /
    //     \ | /
    //      \|/
    //       1

    std::vector<Vector4i> tets = { Vector4i(0, 1, 2, 3) };

    SimplicialTopology topo;
    topo.initFromTetrahedra(4, tets);

    EXPECT_EQ(topo.numVertices(), 4);
    EXPECT_EQ(topo.numTetrahedra(), 1);
    EXPECT_EQ(topo.numTriangles(), 4);  // 4 faces per tet
    EXPECT_EQ(topo.numEdges(), 6);       // 6 edges per tet
    EXPECT_FALSE(topo.is_surface_only);
}

// Two tetrahedra sharing a face
TEST(TopologyTetTest, TwoTetsSharedFace) {
    // Two tets share face (0,1,2)
    // Tet 1: (0,1,2,3) - vertex 3 is "above"
    // Tet 2: (0,1,2,4) - vertex 4 is "below"

    std::vector<Vector4i> tets = {
        Vector4i(0, 1, 2, 3),
        Vector4i(0, 1, 2, 4)
    };

    SimplicialTopology topo;
    topo.initFromTetrahedra(5, tets);

    EXPECT_EQ(topo.numVertices(), 5);
    EXPECT_EQ(topo.numTetrahedra(), 2);
    // 4 faces from first tet + 3 new faces from second = 7
    // (face (0,1,2) is shared but stored as unique triangle)
    EXPECT_EQ(topo.numTriangles(), 7);
}

// ============================================================================
// Surface Extraction Tests
// ============================================================================

// Single tet: all 4 faces are boundary (surface)
TEST(TopologySurfaceTest, SingleTetAllSurface) {
    std::vector<Vector4i> tets = { Vector4i(0, 1, 2, 3) };

    SimplicialTopology topo;
    topo.initFromTetrahedra(4, tets);

    // All 4 faces are on the surface
    EXPECT_EQ(topo.surface_triangles.size(), 4u);
    EXPECT_EQ(topo.surface_triangles_oriented.size(), 4u);
}

// Two tets sharing a face: shared face is internal, not surface
TEST(TopologySurfaceTest, SharedFaceIsInternal) {
    std::vector<Vector4i> tets = {
        Vector4i(0, 1, 2, 3),
        Vector4i(0, 1, 2, 4)
    };

    SimplicialTopology topo;
    topo.initFromTetrahedra(5, tets);

    // Total faces = 7 (4 + 4 - 1 shared = 7 unique)
    // Surface faces = 6 (shared face (0,1,2) is internal)
    EXPECT_EQ(topo.surface_triangles.size(), 6u);
}

// Cube as 5 tetrahedra: should have 12 surface triangles
TEST(TopologySurfaceTest, TetCubeSurface) {
    // Standard BCC5 decomposition of cube
    // 8 vertices at cube corners
    std::vector<Vector4i> tets = {
        Vector4i(0, 1, 3, 4),
        Vector4i(1, 2, 3, 6),
        Vector4i(1, 4, 5, 6),
        Vector4i(3, 4, 6, 7),
        Vector4i(1, 3, 4, 6)  // Central tet
    };

    SimplicialTopology topo;
    topo.initFromTetrahedra(8, tets);

    // Cube has 6 faces, each split into 2 triangles = 12 surface triangles
    EXPECT_EQ(topo.surface_triangles.size(), 12u);
}

// ============================================================================
// Winding Order Tests (critical for rendering)
// ============================================================================

// Surface triangles should have outward-facing normals
TEST(TopologyWindingTest, OutwardNormals) {
    // Create a simple tetrahedron with known geometry
    // Vertices form a right-handed tet with apex at (0.5, 1, 0.5)

    std::vector<Vector3r> verts = {
        Vector3r(0, 0, 0),      // v0
        Vector3r(1, 0, 0),      // v1
        Vector3r(0.5f, 0, 1),   // v2
        Vector3r(0.5f, 1, 0.5f) // v3 (apex)
    };
    std::vector<Vector4i> tets = { Vector4i(0, 1, 2, 3) };

    SimplicialTopology topo;
    topo.initFromTetrahedra(4, tets);

    // Compute centroid
    Vector3r centroid = (verts[0] + verts[1] + verts[2] + verts[3]) / Real(4);

    // Check each surface triangle: normal should point away from centroid
    for (const auto& tri : topo.surface_triangles_oriented) {
        const auto& p0 = verts[tri[0]];
        const auto& p1 = verts[tri[1]];
        const auto& p2 = verts[tri[2]];

        // Face center
        Vector3r face_center = (p0 + p1 + p2) / Real(3);

        // Face normal (cross product gives area-weighted normal)
        Vector3r e1 = p1 - p0;
        Vector3r e2 = p2 - p0;
        Vector3r normal = e1.cross(e2);

        // Vector from centroid to face should align with normal (positive dot)
        Vector3r outward = face_center - centroid;
        Real dot = normal.dot(outward);

        EXPECT_GT(dot, Real(0)) << "Face normal should point outward";
    }
}

// ============================================================================
// Clear and Reinitialize Tests
// ============================================================================

TEST(TopologyClearTest, ClearAndReinit) {
    // Create a triangle mesh
    std::vector<Vector3i> tris = { Vector3i(0, 1, 2) };

    SimplicialTopology topo;
    topo.initFromTriangles(3, tris);

    EXPECT_EQ(topo.numVertices(), 3);
    EXPECT_EQ(topo.numEdges(), 3);

    // Clear
    topo.clear();
    EXPECT_EQ(topo.numVertices(), 0);
    EXPECT_EQ(topo.numEdges(), 0);
    EXPECT_EQ(topo.numTriangles(), 0);

    // Reinitialize with different mesh
    std::vector<Vector4i> tets = { Vector4i(0, 1, 2, 3) };
    topo.initFromTetrahedra(4, tets);

    EXPECT_EQ(topo.numVertices(), 4);
    EXPECT_EQ(topo.numTetrahedra(), 1);
    EXPECT_FALSE(topo.is_surface_only);
}
