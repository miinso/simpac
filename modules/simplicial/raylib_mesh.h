#pragma once

#include "simplicial.h"
#include "topology.h"
#include "mechanical_state.h"
#include <raylib.h>

namespace simplicial {

// ============================================================================
// RaylibMeshCache - Cached Raylib Mesh for Rendering
// ============================================================================
// Attach as component alongside SimplicialTopology + MechanicalState

struct RaylibMeshCache {
    Mesh mesh = {0};
    bool initialized = false;
    bool dirty = true;

    ~RaylibMeshCache() { release(); }

    void release() {
        if (initialized && mesh.vertexCount > 0) {
            UnloadMesh(mesh);
            mesh = {0};
            initialized = false;
        }
    }
};

// ============================================================================
// Factory Functions
// ============================================================================

// Import from raylib Mesh (surface mesh only)
void from_raylib_mesh(
    SimplicialTopology& topo,
    MechanicalState& state,
    const Mesh& rl_mesh);

// ============================================================================
// Raylib Mesh Operations
// ============================================================================

// Create raylib Mesh from surface triangles
// Uses surface_triangles if available, otherwise all triangles
Mesh extract_surface_mesh(
    const SimplicialTopology& topo,
    const MechanicalState& state);

// Update existing raylib mesh positions (for deformation)
// Does NOT reallocate - topology must match
void update_raylib_mesh(
    Mesh& rl_mesh,
    const SimplicialTopology& topo,
    const MechanicalState& state);

// Update vertex normals after position changes
void update_raylib_normals(
    Mesh& rl_mesh,
    const SimplicialTopology& topo,
    const MechanicalState& state);

// Initialize cache from topology + state (allocates GPU buffers)
void init_raylib_cache(
    RaylibMeshCache& cache,
    const SimplicialTopology& topo,
    const MechanicalState& state);

// Sync cache with current positions (updates GPU buffers)
void sync_raylib_cache(
    RaylibMeshCache& cache,
    const SimplicialTopology& topo,
    const MechanicalState& state);

// ============================================================================
// Debug Drawing
// ============================================================================

// Draw mesh wireframe (edges)
void draw_wireframe(
    const SimplicialTopology& topo,
    const MechanicalState& state,
    Color color = DARKGRAY);

// Draw mesh vertices as points
void draw_vertices(
    const MechanicalState& state,
    float radius = 0.02f,
    Color color = RED);

// Draw surface normals
void draw_normals(
    const SimplicialTopology& topo,
    const MechanicalState& state,
    float length = 0.1f,
    Color color = BLUE);

} // namespace simplicial
