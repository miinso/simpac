#include "raylib_mesh.h"
#include <cstring>

namespace simplicial {

// ============================================================================
// Factory Functions
// ============================================================================

void from_raylib_mesh(
    SimplicialTopology& topo,
    MechanicalState& state,
    const Mesh& rl_mesh)
{
    // Extract vertices
    int numVerts = rl_mesh.vertexCount;
    std::vector<Vector3r> vertices(numVerts);

    for (int i = 0; i < numVerts; ++i) {
        vertices[i] = Vector3r(
            static_cast<Real>(rl_mesh.vertices[i * 3 + 0]),
            static_cast<Real>(rl_mesh.vertices[i * 3 + 1]),
            static_cast<Real>(rl_mesh.vertices[i * 3 + 2]));
    }

    // Extract triangles
    std::vector<Vector3i> triangles;

    if (rl_mesh.indices != nullptr) {
        // Indexed mesh
        triangles.resize(rl_mesh.triangleCount);
        for (int i = 0; i < rl_mesh.triangleCount; ++i) {
            triangles[i] = Vector3i(
                static_cast<int>(rl_mesh.indices[i * 3 + 0]),
                static_cast<int>(rl_mesh.indices[i * 3 + 1]),
                static_cast<int>(rl_mesh.indices[i * 3 + 2]));
        }
    } else {
        // Non-indexed mesh (every 3 vertices form a triangle)
        int numTris = numVerts / 3;
        triangles.resize(numTris);
        for (int i = 0; i < numTris; ++i) {
            triangles[i] = Vector3i(i * 3 + 0, i * 3 + 1, i * 3 + 2);
        }
    }

    from_surface_mesh(topo, state, vertices, triangles);
}

// ============================================================================
// Helper: Compute Vertex Normals
// ============================================================================

static void compute_vertex_normals(
    float* normals,
    const SimplicialTopology& topo,
    const MechanicalState& state)
{
    int numVerts = state.size();

    // Clear normals
    std::memset(normals, 0, numVerts * 3 * sizeof(float));

    // Use oriented surface triangles for correct winding
    const auto& surface_tris = topo.surface_triangles_oriented.empty()
        ? topo.triangles
        : topo.surface_triangles_oriented;

    // Accumulate face normals to vertices
    for (const auto& tri : surface_tris) {
        int v0 = tri[0], v1 = tri[1], v2 = tri[2];

        const auto& p0 = state.positions[v0];
        const auto& p1 = state.positions[v1];
        const auto& p2 = state.positions[v2];

        Vector3r e1 = p1 - p0;
        Vector3r e2 = p2 - p0;
        Vector3r n = e1.cross(e2);
        // Don't normalize - larger triangles contribute more (area-weighted)

        for (int vi : {v0, v1, v2}) {
            normals[vi * 3 + 0] += static_cast<float>(n.x());
            normals[vi * 3 + 1] += static_cast<float>(n.y());
            normals[vi * 3 + 2] += static_cast<float>(n.z());
        }
    }

    // Normalize vertex normals
    for (int i = 0; i < numVerts; ++i) {
        float nx = normals[i * 3 + 0];
        float ny = normals[i * 3 + 1];
        float nz = normals[i * 3 + 2];
        float len = std::sqrt(nx * nx + ny * ny + nz * nz);
        if (len > 1e-8f) {
            normals[i * 3 + 0] = nx / len;
            normals[i * 3 + 1] = ny / len;
            normals[i * 3 + 2] = nz / len;
        } else {
            // Default normal for degenerate case
            normals[i * 3 + 0] = 0.0f;
            normals[i * 3 + 1] = 1.0f;
            normals[i * 3 + 2] = 0.0f;
        }
    }
}

// ============================================================================
// Raylib Mesh Operations
// ============================================================================

Mesh extract_surface_mesh(
    const SimplicialTopology& topo,
    const MechanicalState& state)
{
    // Use oriented surface triangles for correct winding
    const auto& surface_tris = topo.surface_triangles_oriented.empty()
        ? topo.triangles
        : topo.surface_triangles_oriented;

    int numTris = static_cast<int>(surface_tris.size());
    int numVerts = state.size();

    Mesh mesh = {0};
    mesh.vertexCount = numVerts;
    mesh.triangleCount = numTris;

    // Allocate CPU buffers
    mesh.vertices = static_cast<float*>(RL_MALLOC(numVerts * 3 * sizeof(float)));
    mesh.normals = static_cast<float*>(RL_MALLOC(numVerts * 3 * sizeof(float)));
    mesh.texcoords = static_cast<float*>(RL_MALLOC(numVerts * 2 * sizeof(float)));
    mesh.indices = static_cast<unsigned short*>(RL_MALLOC(numTris * 3 * sizeof(unsigned short)));

    // Copy vertices
    for (int i = 0; i < numVerts; ++i) {
        const auto& p = state.positions[i];
        mesh.vertices[i * 3 + 0] = static_cast<float>(p.x());
        mesh.vertices[i * 3 + 1] = static_cast<float>(p.y());
        mesh.vertices[i * 3 + 2] = static_cast<float>(p.z());
    }

    // Copy indices with correct winding order
    for (int i = 0; i < numTris; ++i) {
        const auto& tri = surface_tris[i];
        mesh.indices[i * 3 + 0] = static_cast<unsigned short>(tri[0]);
        mesh.indices[i * 3 + 1] = static_cast<unsigned short>(tri[1]);
        mesh.indices[i * 3 + 2] = static_cast<unsigned short>(tri[2]);
    }

    // Compute normals
    compute_vertex_normals(mesh.normals, topo, state);

    // Zero texcoords for now
    std::memset(mesh.texcoords, 0, numVerts * 2 * sizeof(float));

    // Upload to GPU (dynamic = true for updates)
    UploadMesh(&mesh, true);

    return mesh;
}

void update_raylib_mesh(
    Mesh& mesh,
    const SimplicialTopology& topo,
    const MechanicalState& state)
{
    int numVerts = state.size();

    // Update vertex positions
    for (int i = 0; i < numVerts; ++i) {
        const auto& p = state.positions[i];
        mesh.vertices[i * 3 + 0] = static_cast<float>(p.x());
        mesh.vertices[i * 3 + 1] = static_cast<float>(p.y());
        mesh.vertices[i * 3 + 2] = static_cast<float>(p.z());
    }

    // Recompute normals
    update_raylib_normals(mesh, topo, state);

    // Sync to GPU
    UpdateMeshBuffer(mesh, 0, mesh.vertices, numVerts * 3 * sizeof(float), 0);
    UpdateMeshBuffer(mesh, 2, mesh.normals, numVerts * 3 * sizeof(float), 0);
}

void update_raylib_normals(
    Mesh& mesh,
    const SimplicialTopology& topo,
    const MechanicalState& state)
{
    compute_vertex_normals(mesh.normals, topo, state);
}

void init_raylib_cache(
    RaylibMeshCache& cache,
    const SimplicialTopology& topo,
    const MechanicalState& state)
{
    cache.release();
    cache.mesh = extract_surface_mesh(topo, state);
    cache.initialized = true;
    cache.dirty = false;
}

void sync_raylib_cache(
    RaylibMeshCache& cache,
    const SimplicialTopology& topo,
    const MechanicalState& state)
{
    if (!cache.initialized) {
        init_raylib_cache(cache, topo, state);
        return;
    }

    update_raylib_mesh(cache.mesh, topo, state);
    cache.dirty = false;
}

// ============================================================================
// Debug Drawing
// ============================================================================

void draw_wireframe(
    const SimplicialTopology& topo,
    const MechanicalState& state,
    Color color)
{
    for (const auto& edge : topo.edges) {
        const auto& p0 = state.positions[edge[0]];
        const auto& p1 = state.positions[edge[1]];

        DrawLine3D(
            {static_cast<float>(p0.x()), static_cast<float>(p0.y()), static_cast<float>(p0.z())},
            {static_cast<float>(p1.x()), static_cast<float>(p1.y()), static_cast<float>(p1.z())},
            color);
    }
}

void draw_vertices(
    const MechanicalState& state,
    float radius,
    Color color)
{
    for (int i = 0; i < state.size(); ++i) {
        const auto& p = state.positions[i];
        DrawSphere(
            {static_cast<float>(p.x()), static_cast<float>(p.y()), static_cast<float>(p.z())},
            radius,
            color);
    }
}

void draw_normals(
    const SimplicialTopology& topo,
    const MechanicalState& state,
    float length,
    Color color)
{
    // Use oriented surface triangles for correct normals
    const auto& surface_tris = topo.surface_triangles_oriented.empty()
        ? topo.triangles
        : topo.surface_triangles_oriented;

    // Draw face normals at triangle centers
    for (const auto& tri : surface_tris) {
        const auto& p0 = state.positions[tri[0]];
        const auto& p1 = state.positions[tri[1]];
        const auto& p2 = state.positions[tri[2]];

        Vector3r center = (p0 + p1 + p2) / Real(3);
        Vector3r e1 = p1 - p0;
        Vector3r e2 = p2 - p0;
        Vector3r n = e1.cross(e2).normalized();
        Vector3r end = center + n * length;

        DrawLine3D(
            {static_cast<float>(center.x()), static_cast<float>(center.y()), static_cast<float>(center.z())},
            {static_cast<float>(end.x()), static_cast<float>(end.y()), static_cast<float>(end.z())},
            color);
    }
}

} // namespace simplicial
