#pragma once

// Simplicial Mesh Module
// Provides K=(V,E,F,T) simplicial complex for FEM simulation
// SOFA-style hybrid: separate topology and mechanical state components

#include <Eigen/Dense>
#include <flecs.h>
#include <vector>

namespace simplicial {

// ============================================================================
// Type Definitions (aligned with modules/pbd/Common.h)
// ============================================================================

#ifdef USE_DOUBLE
using Real = double;
#else
using Real = float;
#endif

// Scalar vectors
using Vector2r = Eigen::Matrix<Real, 2, 1, Eigen::DontAlign>;
using Vector3r = Eigen::Matrix<Real, 3, 1, Eigen::DontAlign>;
using Vector4r = Eigen::Matrix<Real, 4, 1, Eigen::DontAlign>;

// Index vectors for simplicial elements
using Vector2i = Eigen::Matrix<int, 2, 1, Eigen::DontAlign>;
using Vector3i = Eigen::Matrix<int, 3, 1, Eigen::DontAlign>;
using Vector4i = Eigen::Matrix<int, 4, 1, Eigen::DontAlign>;

// Matrices
using Matrix3r = Eigen::Matrix<Real, 3, 3, Eigen::DontAlign>;
using Matrix4r = Eigen::Matrix<Real, 4, 4, Eigen::DontAlign>;

// ============================================================================
// Forward Declarations
// ============================================================================

struct SimplicialTopology;
struct MechanicalState;
struct RaylibMeshCache;

// ============================================================================
// Flecs Component Registration
// ============================================================================

void register_components(flecs::world& ecs);

// ============================================================================
// Factory Functions
// ============================================================================

// Create topology + state from tet mesh (builds full K=(V,E,F,T))
void from_tet_mesh(
    SimplicialTopology& topo,
    MechanicalState& state,
    const std::vector<Vector3r>& vertices,
    const std::vector<Vector4i>& tetrahedra);

// Create topology + state from surface mesh (builds K=(V,E,F))
void from_surface_mesh(
    SimplicialTopology& topo,
    MechanicalState& state,
    const std::vector<Vector3r>& vertices,
    const std::vector<Vector3i>& triangles);

// ============================================================================
// Flecs Entity Helpers
// ============================================================================

// Create entity with surface mesh components
flecs::entity create_mesh_entity(
    flecs::world& ecs,
    const std::vector<Vector3r>& vertices,
    const std::vector<Vector3i>& triangles,
    const char* name = nullptr);

// Create entity with tet mesh components
flecs::entity create_tet_mesh_entity(
    flecs::world& ecs,
    const std::vector<Vector3r>& vertices,
    const std::vector<Vector4i>& tetrahedra,
    const char* name = nullptr);

} // namespace simplicial
