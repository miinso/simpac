// raylib_integration_test.cpp
// Manual integration test for raylib mesh functions (requires display)
//
// Run with: bazel run //modules/simplicial/tests:raylib_integration_test
//
// This test demonstrates:
// - extract_surface_mesh: Create raylib Mesh from simplicial data
// - update_raylib_mesh: Sync deformed positions to GPU
// - from_raylib_mesh: Import raylib Mesh to simplicial format
// - RaylibMeshCache: Automatic cache management
// - draw_wireframe, draw_vertices, draw_normals: Debug visualization
//
// Controls:
//   SPACE - Toggle animation (mesh deformation)
//   W     - Toggle wireframe
//   V     - Toggle vertices
//   N     - Toggle normals
//   1     - Show tet cube
//   2     - Show raylib sphere (from_raylib_mesh demo)
//   ESC   - Exit

#include "modules/simplicial/simplicial.h"
#include "modules/simplicial/topology.h"
#include "modules/simplicial/mechanical_state.h"
#include "modules/simplicial/raylib_mesh.h"

#include <raylib.h>
#include <raymath.h>
#include <cstdio>
#include <cmath>

using namespace simplicial;

// ============================================================================
// Test Meshes
// ============================================================================

// Create a tet cube (5 tetrahedra)
void create_tet_cube(
    SimplicialTopology& topo,
    MechanicalState& state)
{
    std::vector<Vector3r> vertices = {
        Vector3r(-0.5f, -0.5f, -0.5f),  // 0
        Vector3r( 0.5f, -0.5f, -0.5f),  // 1
        Vector3r( 0.5f,  0.5f, -0.5f),  // 2
        Vector3r(-0.5f,  0.5f, -0.5f),  // 3
        Vector3r(-0.5f, -0.5f,  0.5f),  // 4
        Vector3r( 0.5f, -0.5f,  0.5f),  // 5
        Vector3r( 0.5f,  0.5f,  0.5f),  // 6
        Vector3r(-0.5f,  0.5f,  0.5f)   // 7
    };

    std::vector<Vector4i> tets = {
        Vector4i(0, 1, 3, 4),
        Vector4i(1, 2, 3, 6),
        Vector4i(1, 4, 5, 6),
        Vector4i(3, 4, 6, 7),
        Vector4i(1, 3, 4, 6)  // Central tet
    };

    from_tet_mesh(topo, state, vertices, tets);
}

// ============================================================================
// Main
// ============================================================================

int main() {
    // Initialize window
    const int screenWidth = 1024;
    const int screenHeight = 768;
    InitWindow(screenWidth, screenHeight, "Simplicial Raylib Integration Test");
    SetTargetFPS(60);

    // Setup camera
    Camera3D camera = {0};
    camera.position = Vector3{3.0f, 3.0f, 3.0f};
    camera.target = Vector3{0.0f, 0.0f, 0.0f};
    camera.up = Vector3{0.0f, 1.0f, 0.0f};
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    // ========================================================================
    // Test 1: Create mesh from tet data, render with raylib
    // ========================================================================

    SimplicialTopology topo_cube;
    MechanicalState state_cube;
    create_tet_cube(topo_cube, state_cube);

    printf("Tet Cube Stats:\n");
    printf("  Vertices: %d\n", topo_cube.numVertices());
    printf("  Edges: %d\n", topo_cube.numEdges());
    printf("  Triangles: %d\n", topo_cube.numTriangles());
    printf("  Tetrahedra: %d\n", topo_cube.numTetrahedra());
    printf("  Surface triangles: %zu\n", topo_cube.surface_triangles.size());

    // Create raylib mesh from simplicial data
    Mesh mesh_cube = extract_surface_mesh(topo_cube, state_cube);
    printf("\nRaylib mesh created: %d vertices, %d triangles\n",
           mesh_cube.vertexCount, mesh_cube.triangleCount);

    // ========================================================================
    // Test 2: Import raylib mesh to simplicial (from_raylib_mesh)
    // ========================================================================

    // Generate a sphere using raylib
    Mesh rl_sphere = GenMeshSphere(0.5f, 16, 16);

    SimplicialTopology topo_sphere;
    MechanicalState state_sphere;
    from_raylib_mesh(topo_sphere, state_sphere, rl_sphere);

    printf("\nSphere (from raylib):\n");
    printf("  Vertices: %d\n", topo_sphere.numVertices());
    printf("  Edges: %d\n", topo_sphere.numEdges());
    printf("  Triangles: %d\n", topo_sphere.numTriangles());

    // Create mesh for rendering the sphere
    Mesh mesh_sphere = extract_surface_mesh(topo_sphere, state_sphere);

    // ========================================================================
    // Setup materials
    // ========================================================================

    Material mat_cube = LoadMaterialDefault();
    mat_cube.maps[MATERIAL_MAP_DIFFUSE].color = SKYBLUE;

    Material mat_sphere = LoadMaterialDefault();
    mat_sphere.maps[MATERIAL_MAP_DIFFUSE].color = ORANGE;

    // ========================================================================
    // State
    // ========================================================================

    bool animating = false;
    bool show_wireframe = true;
    bool show_vertices = false;
    bool show_normals = false;
    int current_mesh = 1;  // 1 = cube, 2 = sphere
    float time = 0.0f;

    printf("\nControls:\n");
    printf("  SPACE - Toggle animation\n");
    printf("  W     - Toggle wireframe\n");
    printf("  V     - Toggle vertices\n");
    printf("  N     - Toggle normals\n");
    printf("  1     - Show tet cube\n");
    printf("  2     - Show sphere\n");
    printf("  ESC   - Exit\n");

    // ========================================================================
    // Main loop
    // ========================================================================

    while (!WindowShouldClose()) {
        // Input
        if (IsKeyPressed(KEY_SPACE)) animating = !animating;
        if (IsKeyPressed(KEY_W)) show_wireframe = !show_wireframe;
        if (IsKeyPressed(KEY_V)) show_vertices = !show_vertices;
        if (IsKeyPressed(KEY_N)) show_normals = !show_normals;
        if (IsKeyPressed(KEY_ONE)) current_mesh = 1;
        if (IsKeyPressed(KEY_TWO)) current_mesh = 2;

        UpdateCamera(&camera, CAMERA_ORBITAL);

        // Animation: deform mesh vertices
        if (animating) {
            time += GetFrameTime();

            if (current_mesh == 1) {
                // Deform cube: wave effect
                for (int i = 0; i < state_cube.size(); ++i) {
                    const auto& rest = state_cube.rest_positions[i];
                    Real wave = Real(0.1) * std::sin(time * 3.0f + rest.x() * 4.0f);
                    state_cube.positions[i] = rest + Vector3r(0, wave, 0);
                }
                // Update GPU buffers
                update_raylib_mesh(mesh_cube, topo_cube, state_cube);
            } else {
                // Deform sphere: pulsing effect
                for (int i = 0; i < state_sphere.size(); ++i) {
                    const auto& rest = state_sphere.rest_positions[i];
                    Real scale = Real(1.0) + Real(0.2) * std::sin(time * 2.0f);
                    state_sphere.positions[i] = rest * scale;
                }
                update_raylib_mesh(mesh_sphere, topo_sphere, state_sphere);
            }
        }

        // Draw
        BeginDrawing();
        ClearBackground(RAYWHITE);

        BeginMode3D(camera);

        // Draw grid
        DrawGrid(10, 1.0f);

        if (current_mesh == 1) {
            // Draw cube
            DrawMesh(mesh_cube, mat_cube, MatrixIdentity());

            if (show_wireframe) {
                draw_wireframe(topo_cube, state_cube, DARKBLUE);
            }
            if (show_vertices) {
                draw_vertices(state_cube, 0.03f, RED);
            }
            if (show_normals) {
                draw_normals(topo_cube, state_cube, 0.15f, GREEN);
            }
        } else {
            // Draw sphere
            DrawMesh(mesh_sphere, mat_sphere, MatrixIdentity());

            if (show_wireframe) {
                draw_wireframe(topo_sphere, state_sphere, MAROON);
            }
            if (show_vertices) {
                draw_vertices(state_sphere, 0.02f, RED);
            }
            if (show_normals) {
                draw_normals(topo_sphere, state_sphere, 0.1f, GREEN);
            }
        }

        EndMode3D();

        // UI
        DrawText("Simplicial Raylib Integration Test", 10, 10, 20, DARKGRAY);

        const char* mesh_name = (current_mesh == 1) ? "Tet Cube" : "Sphere (from_raylib_mesh)";
        DrawText(TextFormat("Mesh: %s", mesh_name), 10, 40, 16, DARKGRAY);
        DrawText(TextFormat("Animation: %s", animating ? "ON" : "OFF"), 10, 60, 16, DARKGRAY);
        DrawText(TextFormat("Wireframe [W]: %s", show_wireframe ? "ON" : "OFF"), 10, 80, 16, DARKGRAY);
        DrawText(TextFormat("Vertices [V]: %s", show_vertices ? "ON" : "OFF"), 10, 100, 16, DARKGRAY);
        DrawText(TextFormat("Normals [N]: %s", show_normals ? "ON" : "OFF"), 10, 120, 16, DARKGRAY);

        DrawFPS(screenWidth - 100, 10);

        EndDrawing();
    }

    // Cleanup
    UnloadMesh(mesh_cube);
    UnloadMesh(mesh_sphere);
    UnloadMesh(rl_sphere);
    UnloadMaterial(mat_cube);
    UnloadMaterial(mat_sphere);

    CloseWindow();

    printf("\nTest completed successfully.\n");
    return 0;
}
