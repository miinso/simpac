// factory_test.cpp
// Tests for factory functions and Flecs integration
//
// Usage examples embedded in tests demonstrate:
// - from_surface_mesh: Create mesh from vertices + triangles
// - from_tet_mesh: Create volumetric mesh from vertices + tetrahedra
// - create_mesh_entity: Create Flecs entity with mesh components
// - create_tet_mesh_entity: Create Flecs entity with tet mesh

#include "modules/simplicial/simplicial.h"
#include "modules/simplicial/topology.h"
#include "modules/simplicial/mechanical_state.h"
#include <flecs.h>
#include <gtest/gtest.h>

using namespace simplicial;

// ============================================================================
// from_surface_mesh Tests
// ============================================================================

// from_surface_mesh populates both topology and state
TEST(FactorySurfaceMeshTest, PopulatesBothComponents) {
    // Simple triangle
    std::vector<Vector3r> vertices = {
        Vector3r(0, 0, 0),
        Vector3r(1, 0, 0),
        Vector3r(0.5f, 1, 0)
    };
    std::vector<Vector3i> triangles = { Vector3i(0, 1, 2) };

    SimplicialTopology topo;
    MechanicalState state;

    from_surface_mesh(topo, state, vertices, triangles);

    // Topology is built
    EXPECT_EQ(topo.numVertices(), 3);
    EXPECT_EQ(topo.numTriangles(), 1);
    EXPECT_EQ(topo.numEdges(), 3);
    EXPECT_TRUE(topo.is_surface_only);

    // State is populated
    EXPECT_EQ(state.size(), 3);
    EXPECT_FLOAT_EQ(state.positions[0].x(), 0.0f);
    EXPECT_FLOAT_EQ(state.positions[1].x(), 1.0f);
    EXPECT_FLOAT_EQ(state.positions[2].y(), 1.0f);
}

// Positions are copied to both current and rest
TEST(FactorySurfaceMeshTest, CopiesRestPositions) {
    std::vector<Vector3r> vertices = {
        Vector3r(1, 2, 3),
        Vector3r(4, 5, 6),
        Vector3r(7, 8, 9)
    };
    std::vector<Vector3i> triangles = { Vector3i(0, 1, 2) };

    SimplicialTopology topo;
    MechanicalState state;

    from_surface_mesh(topo, state, vertices, triangles);

    // Rest positions match input
    EXPECT_FLOAT_EQ(state.rest_positions[0].x(), 1.0f);
    EXPECT_FLOAT_EQ(state.rest_positions[1].y(), 5.0f);
    EXPECT_FLOAT_EQ(state.rest_positions[2].z(), 9.0f);
}

// Quad mesh (2 triangles)
TEST(FactorySurfaceMeshTest, QuadMesh) {
    // Quad vertices
    std::vector<Vector3r> vertices = {
        Vector3r(0, 0, 0),
        Vector3r(1, 0, 0),
        Vector3r(1, 1, 0),
        Vector3r(0, 1, 0)
    };
    std::vector<Vector3i> triangles = {
        Vector3i(0, 1, 2),
        Vector3i(0, 2, 3)
    };

    SimplicialTopology topo;
    MechanicalState state;

    from_surface_mesh(topo, state, vertices, triangles);

    EXPECT_EQ(topo.numVertices(), 4);
    EXPECT_EQ(topo.numTriangles(), 2);
    EXPECT_EQ(topo.numEdges(), 5);  // 4 boundary + 1 diagonal
    EXPECT_EQ(state.size(), 4);
}

// ============================================================================
// from_tet_mesh Tests
// ============================================================================

// from_tet_mesh builds full K=(V,E,F,T) complex
TEST(FactoryTetMeshTest, BuildsFullComplex) {
    // Simple tetrahedron
    std::vector<Vector3r> vertices = {
        Vector3r(0, 0, 0),
        Vector3r(1, 0, 0),
        Vector3r(0.5f, 0, 1),
        Vector3r(0.5f, 1, 0.5f)
    };
    std::vector<Vector4i> tets = { Vector4i(0, 1, 2, 3) };

    SimplicialTopology topo;
    MechanicalState state;

    from_tet_mesh(topo, state, vertices, tets);

    // Full complex
    EXPECT_EQ(topo.numVertices(), 4);
    EXPECT_EQ(topo.numEdges(), 6);
    EXPECT_EQ(topo.numTriangles(), 4);
    EXPECT_EQ(topo.numTetrahedra(), 1);
    EXPECT_FALSE(topo.is_surface_only);

    // State populated
    EXPECT_EQ(state.size(), 4);
}

// Surface extraction works
TEST(FactoryTetMeshTest, ExtractsSurface) {
    std::vector<Vector3r> vertices = {
        Vector3r(0, 0, 0),
        Vector3r(1, 0, 0),
        Vector3r(0.5f, 0, 1),
        Vector3r(0.5f, 1, 0.5f)
    };
    std::vector<Vector4i> tets = { Vector4i(0, 1, 2, 3) };

    SimplicialTopology topo;
    MechanicalState state;

    from_tet_mesh(topo, state, vertices, tets);

    // All 4 faces are surface for single tet
    EXPECT_EQ(topo.surface_triangles.size(), 4u);
    EXPECT_EQ(topo.surface_triangles_oriented.size(), 4u);
}

// Tet cube (5 tets)
TEST(FactoryTetMeshTest, TetCube) {
    // Cube corners
    std::vector<Vector3r> vertices = {
        Vector3r(0, 0, 0),  // 0
        Vector3r(1, 0, 0),  // 1
        Vector3r(1, 1, 0),  // 2
        Vector3r(0, 1, 0),  // 3
        Vector3r(0, 0, 1),  // 4
        Vector3r(1, 0, 1),  // 5
        Vector3r(1, 1, 1),  // 6
        Vector3r(0, 1, 1)   // 7
    };
    // BCC5 decomposition
    std::vector<Vector4i> tets = {
        Vector4i(0, 1, 3, 4),
        Vector4i(1, 2, 3, 6),
        Vector4i(1, 4, 5, 6),
        Vector4i(3, 4, 6, 7),
        Vector4i(1, 3, 4, 6)
    };

    SimplicialTopology topo;
    MechanicalState state;

    from_tet_mesh(topo, state, vertices, tets);

    EXPECT_EQ(topo.numVertices(), 8);
    EXPECT_EQ(topo.numTetrahedra(), 5);
    EXPECT_EQ(topo.surface_triangles.size(), 12u);  // 6 faces * 2 tris
}

// ============================================================================
// Flecs Entity Creation Tests
// ============================================================================

// create_mesh_entity creates entity with components
TEST(FlecsIntegrationTest, CreateMeshEntity) {
    flecs::world ecs;
    register_components(ecs);

    std::vector<Vector3r> verts = {
        Vector3r(0, 0, 0),
        Vector3r(1, 0, 0),
        Vector3r(0, 1, 0)
    };
    std::vector<Vector3i> tris = { Vector3i(0, 1, 2) };

    // Create entity
    auto entity = create_mesh_entity(ecs, verts, tris, "test_triangle");

    // Entity has both components
    EXPECT_TRUE(entity.has<SimplicialTopology>());
    EXPECT_TRUE(entity.has<MechanicalState>());

    // Components are populated
    const auto& topo = entity.get<SimplicialTopology>();
    const auto& state = entity.get<MechanicalState>();

    EXPECT_EQ(topo.numTriangles(), 1);
    EXPECT_EQ(state.size(), 3);
}

// Entity name is set correctly
TEST(FlecsIntegrationTest, EntityHasName) {
    flecs::world ecs;
    register_components(ecs);

    std::vector<Vector3r> verts = {
        Vector3r(0, 0, 0),
        Vector3r(1, 0, 0),
        Vector3r(0, 1, 0)
    };
    std::vector<Vector3i> tris = { Vector3i(0, 1, 2) };

    auto entity = create_mesh_entity(ecs, verts, tris, "my_mesh");

    // Check name
    EXPECT_STREQ(entity.name().c_str(), "my_mesh");
}

// Null name creates unnamed entity
TEST(FlecsIntegrationTest, NullNameAllowed) {
    flecs::world ecs;
    register_components(ecs);

    std::vector<Vector3r> verts = {
        Vector3r(0, 0, 0),
        Vector3r(1, 0, 0),
        Vector3r(0, 1, 0)
    };
    std::vector<Vector3i> tris = { Vector3i(0, 1, 2) };

    // Should not crash
    auto entity = create_mesh_entity(ecs, verts, tris, nullptr);

    EXPECT_TRUE(entity.is_valid());
    EXPECT_TRUE(entity.has<SimplicialTopology>());
}

// create_tet_mesh_entity for volumetric meshes
TEST(FlecsIntegrationTest, CreateTetMeshEntity) {
    flecs::world ecs;
    register_components(ecs);

    std::vector<Vector3r> verts = {
        Vector3r(0, 0, 0),
        Vector3r(1, 0, 0),
        Vector3r(0.5f, 0, 1),
        Vector3r(0.5f, 1, 0.5f)
    };
    std::vector<Vector4i> tets = { Vector4i(0, 1, 2, 3) };

    auto entity = create_tet_mesh_entity(ecs, verts, tets, "test_tet");

    const auto& topo = entity.get<SimplicialTopology>();

    EXPECT_EQ(topo.numTetrahedra(), 1);
    EXPECT_EQ(topo.surface_triangles.size(), 4u);
    EXPECT_FALSE(topo.is_surface_only);
}

// ============================================================================
// Query Pattern Tests (common usage)
// ============================================================================

// Query all mesh entities
TEST(FlecsIntegrationTest, QueryMeshEntities) {
    flecs::world ecs;
    register_components(ecs);

    // Create multiple mesh entities
    std::vector<Vector3r> verts = {
        Vector3r(0, 0, 0),
        Vector3r(1, 0, 0),
        Vector3r(0, 1, 0)
    };
    std::vector<Vector3i> tris = { Vector3i(0, 1, 2) };

    create_mesh_entity(ecs, verts, tris, "mesh1");
    create_mesh_entity(ecs, verts, tris, "mesh2");
    create_mesh_entity(ecs, verts, tris, "mesh3");

    // Count mesh entities
    int count = 0;
    ecs.each([&count](const SimplicialTopology&, const MechanicalState&) {
        count++;
    });

    EXPECT_EQ(count, 3);
}

// Modify mesh state through entity
TEST(FlecsIntegrationTest, ModifyMeshState) {
    flecs::world ecs;
    register_components(ecs);

    std::vector<Vector3r> verts = {
        Vector3r(0, 0, 0),
        Vector3r(1, 0, 0),
        Vector3r(0, 1, 0)
    };
    std::vector<Vector3i> tris = { Vector3i(0, 1, 2) };

    auto entity = create_mesh_entity(ecs, verts, tris, "deformable");

    // Get mutable reference to state
    auto& state = entity.ensure<MechanicalState>();

    // Deform the mesh
    state.positions[0] += Vector3r(0.1f, 0, 0);

    // Verify modification persists
    const auto& state_check = entity.get<MechanicalState>();
    EXPECT_FLOAT_EQ(state_check.positions[0].x(), 0.1f);
}
