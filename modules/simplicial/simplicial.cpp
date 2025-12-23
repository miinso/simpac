#include "simplicial.h"
#include "topology.h"
#include "mechanical_state.h"
#include "raylib_mesh.h"

namespace simplicial {

// ============================================================================
// Flecs Component Registration
// ============================================================================

void register_components(flecs::world& ecs) {
    ecs.component<SimplicialTopology>();
    ecs.component<MechanicalState>();
    ecs.component<RaylibMeshCache>();
}

// ============================================================================
// Factory Functions
// ============================================================================

void from_tet_mesh(
    SimplicialTopology& topo,
    MechanicalState& state,
    const std::vector<Vector3r>& vertices,
    const std::vector<Vector4i>& tetrahedra)
{
    int numVerts = static_cast<int>(vertices.size());

    // Initialize topology
    topo.initFromTetrahedra(numVerts, tetrahedra);

    // Initialize state
    state.resize(numVerts);
    for (int i = 0; i < numVerts; ++i) {
        state.positions[i] = vertices[i];
        state.rest_positions[i] = vertices[i];
    }
}

void from_surface_mesh(
    SimplicialTopology& topo,
    MechanicalState& state,
    const std::vector<Vector3r>& vertices,
    const std::vector<Vector3i>& triangles)
{
    int numVerts = static_cast<int>(vertices.size());

    // Initialize topology
    topo.initFromTriangles(numVerts, triangles);

    // Initialize state
    state.resize(numVerts);
    for (int i = 0; i < numVerts; ++i) {
        state.positions[i] = vertices[i];
        state.rest_positions[i] = vertices[i];
    }
}

// ============================================================================
// Flecs Entity Helpers
// ============================================================================

flecs::entity create_mesh_entity(
    flecs::world& ecs,
    const std::vector<Vector3r>& vertices,
    const std::vector<Vector3i>& triangles,
    const char* name)
{
    auto e = ecs.entity();
    if (name) e.set_name(name);

    SimplicialTopology topo;
    MechanicalState state;
    from_surface_mesh(topo, state, vertices, triangles);

    e.set<SimplicialTopology>(std::move(topo));
    e.set<MechanicalState>(std::move(state));

    return e;
}

flecs::entity create_tet_mesh_entity(
    flecs::world& ecs,
    const std::vector<Vector3r>& vertices,
    const std::vector<Vector4i>& tetrahedra,
    const char* name)
{
    auto e = ecs.entity();
    if (name) e.set_name(name);

    SimplicialTopology topo;
    MechanicalState state;
    from_tet_mesh(topo, state, vertices, tetrahedra);

    e.set<SimplicialTopology>(std::move(topo));
    e.set<MechanicalState>(std::move(state));

    return e;
}

} // namespace simplicial
