#include "graphics.h"
#include "modules/simplicial/simplicial.h"
#include "modules/simplicial/topology.h"
#include "modules/simplicial/mechanical_state.h"
#include "modules/simplicial/raylib_mesh.h"

#include <cstdio>

using namespace simplicial;

// Create a simple tetrahedron mesh
void create_single_tet(
    std::vector<Vector3r>& vertices,
    std::vector<Vector4i>& tetrahedra)
{
    // Single tetrahedron
    vertices = {
        Vector3r(0.0f, 0.0f, 0.0f),
        Vector3r(1.0f, 0.0f, 0.0f),
        Vector3r(0.5f, 0.0f, 1.0f),
        Vector3r(0.5f, 1.0f, 0.5f)
    };

    tetrahedra = {
        Vector4i(0, 1, 2, 3)
    };
}

// Create a simple cube as 5 tetrahedra
void create_tet_cube(
    std::vector<Vector3r>& vertices,
    std::vector<Vector4i>& tetrahedra)
{
    // Cube vertices
    vertices = {
        Vector3r(0, 0, 0),  // 0
        Vector3r(1, 0, 0),  // 1
        Vector3r(1, 1, 0),  // 2
        Vector3r(0, 1, 0),  // 3
        Vector3r(0, 0, 1),  // 4
        Vector3r(1, 0, 1),  // 5
        Vector3r(1, 1, 1),  // 6
        Vector3r(0, 1, 1)   // 7
    };

    // 5-tet decomposition of cube
    tetrahedra = {
        Vector4i(0, 1, 3, 4),
        Vector4i(1, 2, 3, 6),
        Vector4i(1, 4, 5, 6),
        Vector4i(3, 4, 6, 7),
        Vector4i(1, 3, 4, 6)
    };
}

int main() {
    flecs::world ecs;

    graphics::init(ecs);
    graphics::init_window(800, 600, "Simplicial Mesh Test");
    graphics::set_draw_grid(true);
    graphics::set_draw_fps(true);

    // Register simplicial components
    register_components(ecs);

    // Create mesh data
    std::vector<Vector3r> vertices;
    std::vector<Vector4i> tetrahedra;
    create_tet_cube(vertices, tetrahedra);

    // Center the mesh
    Vector3r center(0.5f, 0.5f, 0.5f);
    for (auto& v : vertices) {
        v -= center;
    }

    // Create mesh entity
    auto mesh_entity = create_tet_mesh_entity(ecs, vertices, tetrahedra, "cube");
    mesh_entity.add<RaylibMeshCache>();

    // Get component reference for queries (new flecs API)
    const SimplicialTopology& topo = mesh_entity.get<SimplicialTopology>();

    std::printf("Mesh stats:\n");
    std::printf("  Vertices: %d\n", topo.numVertices());
    std::printf("  Edges: %d\n", topo.numEdges());
    std::printf("  Triangles: %d\n", topo.numTriangles());
    std::printf("  Tetrahedra: %d\n", topo.numTetrahedra());
    std::printf("  Surface triangles: %zu\n", topo.surface_triangles.size());

    // Print adjacency info
    std::printf("\nVertex 0 adjacent to:\n");
    std::printf("  Edges: ");
    for (size_t i = 0; i < topo.vert_to_edges[0].size(); ++i) std::printf("%d ", topo.vert_to_edges[0][i]);
    std::printf("\n  Triangles: ");
    for (size_t i = 0; i < topo.vert_to_tris[0].size(); ++i) std::printf("%d ", topo.vert_to_tris[0][i]);
    std::printf("\n  Tetrahedra: ");
    for (size_t i = 0; i < topo.vert_to_tets[0].size(); ++i) std::printf("%d ", topo.vert_to_tets[0][i]);
    std::printf("\n");

    // Load default material
    Material material = LoadMaterialDefault();
    material.maps[MATERIAL_MAP_DIFFUSE].color = SKYBLUE;

    // Render system
    ecs.system<const SimplicialTopology, const MechanicalState, RaylibMeshCache>("RenderMesh")
        .kind(graphics::phase_on_render)
        .each([&material](const SimplicialTopology& topo,
                          const MechanicalState& state,
                          RaylibMeshCache& cache) {
            // Initialize cache if needed
            if (!cache.initialized) {
                init_raylib_cache(cache, topo, state);
            }

            // Draw mesh
            DrawMesh(cache.mesh, material, MatrixIdentity());

            // Draw wireframe
            draw_wireframe(topo, state, DARKBLUE);

            // Draw vertices
            draw_vertices(state, 0.03f, RED);
        });

    std::printf("\nRunning simulation...\n");
    std::printf("Press ESC to exit\n");

    graphics::run_main_loop([]() {});

    return 0;
}
