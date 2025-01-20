#include "geometry/components.hpp"
#include "geometry/operations.hpp"
#include "raylib.h"
#include <gtest/gtest.h>

using namespace phys;
using namespace phys::components;

class GeometryComponentTest : public ::testing::Test
{
protected:
    MeshFilter filter;
    MeshRenderer renderer;

    void SetUp() override {
        // gpu ops requires context
        SetTraceLogLevel(LOG_WARNING);
        InitWindow(100, 100, "context");

        // Create a simple cube mesh for testing
        Mesh cube = GenMeshCube(1.0f, 1.0f, 1.0f);
        filter.init_mesh(cube);
        UnloadMesh(cube);
    }

    void TearDown() override {
        renderer.release_resources();
        // CloseWindow();
    }
};

TEST_F(GeometryComponentTest, MeshFilterInitialization) {
    EXPECT_EQ(filter.get_vertex_count(), 24); // Cube has 24 vertices (6 faces * 4 vertices)
    EXPECT_EQ(filter.get_face_count(), 12); // Cube has 12 triangles (6 faces * 2 triangles)
    EXPECT_GT(filter.get_edge_count(), 0); // Should have edges after initialization
}

TEST_F(GeometryComponentTest, MeshTransformation) {
    Vector3r position(1.0f, 2.0f, 3.0f);
    Quaternionr orientation = Quaternionr::Identity();
    Vector3r scale = Vector3r::Ones();

    // Store original positions
    std::vector<Vector3r> original_positions;
    for (size_t i = 0; i < filter.local_vertices.size(); ++i) {
        original_positions.push_back(filter.local_vertices.getPosition(i));
    }

    // Transform vertices
    filter.transform_vertices(position, orientation, scale, filter);

    // Check if vertices were transformed correctly
    for (size_t i = 0; i < filter.world_vertices.size(); ++i) {
        Vector3r transformed = filter.world_vertices.getPosition(i);
        Vector3r expected = original_positions[i] + position;
        EXPECT_NEAR((transformed - expected).norm(), 0.0f, 1e-6f);
    }
}

TEST_F(GeometryComponentTest, MeshRendererInitialization) {
    EXPECT_FALSE(renderer.is_initialized);
    EXPECT_TRUE(renderer.is_dirty);

    renderer.initialize_renderable(filter);

    EXPECT_TRUE(renderer.is_initialized);
    EXPECT_FALSE(renderer.is_dirty);
    EXPECT_NE(renderer.renderable.vboId, nullptr);
    EXPECT_EQ(renderer.renderable.vertexCount, filter.get_vertex_count());
}

TEST_F(GeometryComponentTest, MeshRendererUpdate) {
    renderer.initialize_renderable(filter);

    // Transform the mesh
    Vector3r position(1.0f, 0.0f, 0.0f);
    Quaternionr orientation = Quaternionr::Identity();
    Vector3r scale = Vector3r::Ones();

    filter.transform_vertices(position, orientation, scale, filter);
    renderer.is_dirty = true;

    // Update the renderable
    renderer.update_renderable(filter);

    // Check if vertices were updated in the renderable
    for (size_t i = 0; i < filter.world_vertices.size(); ++i) {
        Vector3r world_pos = filter.world_vertices.getPosition(i);
        float* vertex = &renderer.renderable.vertices[i * 3];
        EXPECT_NEAR(vertex[0], world_pos.x(), 1e-6f);
        EXPECT_NEAR(vertex[1], world_pos.y(), 1e-6f);
        EXPECT_NEAR(vertex[2], world_pos.z(), 1e-6f);
    }
}

TEST_F(GeometryComponentTest, MeshScaling) {
    Vector3r position = Vector3r::Zero();
    Quaternionr orientation = Quaternionr::Identity();
    Vector3r scale(2.0f, 2.0f, 2.0f);

    // Store original positions
    std::vector<Vector3r> original_positions;
    for (size_t i = 0; i < filter.local_vertices.size(); ++i) {
        original_positions.push_back(filter.local_vertices.getPosition(i));
    }

    // Transform vertices with scaling
    filter.transform_vertices(position, orientation, scale, filter);

    // Verify that normals are still normalized after scaling
    const auto& normals = filter.mesh.getVertexNormals();
    for (const auto& normal : normals) {
        EXPECT_NEAR(normal.norm(), 1.0f, 1e-6f);
    }
}

TEST_F(GeometryComponentTest, MeshRotation) {
    Vector3r position = Vector3r::Zero();
    // 90 degrees rotation around Y axis

    Quaternionr orientation(AngleAxisr(PI / 2, Vector3r::UnitY()));
    Vector3r scale = Vector3r::Ones();

    // Get a vertex that we know should be affected by rotation (e.g., on x-axis)
    Vector3r original = filter.local_vertices.getPosition(0);

    filter.transform_vertices(position, orientation, scale, filter);

    Vector3r rotated = filter.world_vertices.getPosition(0);

    // Check if rotation was applied correctly
    // The x coordinate should now be approximately z, and z should be approximately -x
    if (std::abs(original.x()) > 1e-6f) {
        EXPECT_NEAR(rotated.z(), -original.x(), 1e-6f);
        EXPECT_NEAR(rotated.x(), original.z(), 1e-6f);
    }
}