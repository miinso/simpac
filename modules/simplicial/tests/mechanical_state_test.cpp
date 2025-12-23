// mechanical_state_test.cpp
// Tests for MechanicalState - per-vertex DOF state
//
// Usage examples embedded in tests demonstrate:
// - Initializing state for N vertices
// - Setting and querying positions, velocities
// - Mass handling (setMass, getInvMass, pin/unpin)
// - State operations (saveToOld, computeVelocities)
// - Bulk operations for FEM solvers (positionMatrix, etc.)

#include "modules/simplicial/simplicial.h"
#include "modules/simplicial/mechanical_state.h"
#include <gtest/gtest.h>
#include <cmath>

using namespace simplicial;

// ============================================================================
// Initialization Tests
// ============================================================================

// Basic resize allocates all arrays
TEST(MechanicalStateInitTest, Resize) {
    MechanicalState state;

    state.resize(4);

    EXPECT_EQ(state.size(), 4);
    EXPECT_EQ(state.positions.size(), 4u);
    EXPECT_EQ(state.rest_positions.size(), 4u);
    EXPECT_EQ(state.old_positions.size(), 4u);
    EXPECT_EQ(state.velocities.size(), 4u);
    EXPECT_EQ(state.accelerations.size(), 4u);
    EXPECT_EQ(state.masses.size(), 4u);
    EXPECT_EQ(state.inv_masses.size(), 4u);
}

// Default values after resize
TEST(MechanicalStateInitTest, DefaultValues) {
    MechanicalState state;
    state.resize(2);

    // Positions default to zero
    EXPECT_FLOAT_EQ(state.positions[0].x(), 0.0f);
    EXPECT_FLOAT_EQ(state.positions[0].y(), 0.0f);
    EXPECT_FLOAT_EQ(state.positions[0].z(), 0.0f);

    // Mass defaults to 1
    EXPECT_FLOAT_EQ(state.masses[0], 1.0f);
    EXPECT_FLOAT_EQ(state.inv_masses[0], 1.0f);
}

// Clear removes all data
TEST(MechanicalStateInitTest, Clear) {
    MechanicalState state;
    state.resize(4);

    state.clear();

    EXPECT_EQ(state.size(), 0);
    EXPECT_TRUE(state.positions.empty());
    EXPECT_TRUE(state.masses.empty());
}

// ============================================================================
// Position Accessor Tests
// ============================================================================

// getPosition and setPosition
TEST(MechanicalStateAccessorTest, PositionAccessors) {
    MechanicalState state;
    state.resize(3);

    // Set position
    state.setPosition(0, Vector3r(1, 2, 3));

    // Get position (const and non-const)
    EXPECT_FLOAT_EQ(state.getPosition(0).x(), 1.0f);
    EXPECT_FLOAT_EQ(state.getPosition(0).y(), 2.0f);
    EXPECT_FLOAT_EQ(state.getPosition(0).z(), 3.0f);

    // Modify via non-const reference
    state.getPosition(1) = Vector3r(4, 5, 6);
    EXPECT_FLOAT_EQ(state.positions[1].x(), 4.0f);
}

// Rest position (x0) - reference configuration for FEM
TEST(MechanicalStateAccessorTest, RestPositionAccessors) {
    MechanicalState state;
    state.resize(2);

    state.setPosition0(0, Vector3r(1, 0, 0));
    state.setPosition0(1, Vector3r(0, 1, 0));

    EXPECT_FLOAT_EQ(state.getPosition0(0).x(), 1.0f);
    EXPECT_FLOAT_EQ(state.getPosition0(1).y(), 1.0f);
}

// Velocity accessors
TEST(MechanicalStateAccessorTest, VelocityAccessors) {
    MechanicalState state;
    state.resize(2);

    state.setVelocity(0, Vector3r(1, 0, 0));

    EXPECT_FLOAT_EQ(state.getVelocity(0).x(), 1.0f);
    EXPECT_FLOAT_EQ(state.getVelocity(0).y(), 0.0f);
}

// ============================================================================
// Mass Handling Tests
// ============================================================================

// setMass computes inverse mass
TEST(MechanicalStateMassTest, SetMassComputesInverse) {
    MechanicalState state;
    state.resize(2);

    state.setMass(0, 2.0f);

    EXPECT_FLOAT_EQ(state.getMass(0), 2.0f);
    EXPECT_FLOAT_EQ(state.getInvMass(0), 0.5f);
}

// Zero mass (infinite) gives zero inverse mass
TEST(MechanicalStateMassTest, ZeroMassHandling) {
    MechanicalState state;
    state.resize(1);

    state.setMass(0, 0.0f);

    EXPECT_FLOAT_EQ(state.getMass(0), 0.0f);
    EXPECT_FLOAT_EQ(state.getInvMass(0), 0.0f);
}

// Pin vertex (infinite mass, won't move in simulation)
TEST(MechanicalStateMassTest, PinVertex) {
    MechanicalState state;
    state.resize(3);

    // Set regular mass first
    state.setMass(0, 1.0f);
    EXPECT_FLOAT_EQ(state.getInvMass(0), 1.0f);

    // Pin it
    state.pin(0);
    EXPECT_FLOAT_EQ(state.getInvMass(0), 0.0f);

    // Mass is unchanged (pin only affects inv_mass)
    // This allows "unpinning" to restore previous mass behavior
}

// Unpin vertex restores mass
TEST(MechanicalStateMassTest, UnpinVertex) {
    MechanicalState state;
    state.resize(1);

    state.pin(0);
    EXPECT_FLOAT_EQ(state.getInvMass(0), 0.0f);

    state.unpin(0, 4.0f);
    EXPECT_FLOAT_EQ(state.getMass(0), 4.0f);
    EXPECT_FLOAT_EQ(state.getInvMass(0), 0.25f);
}

// ============================================================================
// State Operations Tests
// ============================================================================

// setRestFromCurrent copies positions to rest_positions
TEST(MechanicalStateOpsTest, SetRestFromCurrent) {
    MechanicalState state;
    state.resize(2);

    state.positions[0] = Vector3r(1, 2, 3);
    state.positions[1] = Vector3r(4, 5, 6);

    state.setRestFromCurrent();

    EXPECT_FLOAT_EQ(state.rest_positions[0].x(), 1.0f);
    EXPECT_FLOAT_EQ(state.rest_positions[1].z(), 6.0f);
}

// saveToOld copies positions to old_positions
TEST(MechanicalStateOpsTest, SaveToOld) {
    MechanicalState state;
    state.resize(2);

    state.positions[0] = Vector3r(1, 2, 3);
    state.saveToOld();

    // Modify position
    state.positions[0] = Vector3r(4, 5, 6);

    // Old should still have saved value
    EXPECT_FLOAT_EQ(state.old_positions[0].x(), 1.0f);
    EXPECT_FLOAT_EQ(state.old_positions[0].y(), 2.0f);
    EXPECT_FLOAT_EQ(state.old_positions[0].z(), 3.0f);
}

// computeVelocities from position difference
TEST(MechanicalStateOpsTest, ComputeVelocities) {
    MechanicalState state;
    state.resize(1);

    // Start at origin
    state.old_positions[0] = Vector3r(0, 0, 0);

    // Move to (1, 0, 0) over dt=0.5
    state.positions[0] = Vector3r(1, 0, 0);

    state.computeVelocities(0.5f);

    // v = (x - x_old) / dt = (1,0,0) / 0.5 = (2,0,0)
    EXPECT_FLOAT_EQ(state.velocities[0].x(), 2.0f);
    EXPECT_FLOAT_EQ(state.velocities[0].y(), 0.0f);
    EXPECT_FLOAT_EQ(state.velocities[0].z(), 0.0f);
}

// clearAccelerations sets all accelerations to zero
TEST(MechanicalStateOpsTest, ClearAccelerations) {
    MechanicalState state;
    state.resize(2);

    state.accelerations[0] = Vector3r(1, 2, 3);
    state.accelerations[1] = Vector3r(4, 5, 6);

    state.clearAccelerations();

    EXPECT_FLOAT_EQ(state.accelerations[0].norm(), 0.0f);
    EXPECT_FLOAT_EQ(state.accelerations[1].norm(), 0.0f);
}

// ============================================================================
// Bulk Operations Tests (for FEM solvers)
// ============================================================================

// positionMatrix returns Nx3 view of positions
TEST(MechanicalStateBulkTest, PositionMatrix) {
    MechanicalState state;
    state.resize(3);

    state.positions[0] = Vector3r(1, 0, 0);
    state.positions[1] = Vector3r(0, 1, 0);
    state.positions[2] = Vector3r(0, 0, 1);

    auto X = state.positionMatrix();

    EXPECT_EQ(X.rows(), 3);
    EXPECT_EQ(X.cols(), 3);

    // Check values
    EXPECT_FLOAT_EQ(X(0, 0), 1.0f);  // x of vertex 0
    EXPECT_FLOAT_EQ(X(1, 1), 1.0f);  // y of vertex 1
    EXPECT_FLOAT_EQ(X(2, 2), 1.0f);  // z of vertex 2

    // Modify via matrix view
    X(0, 1) = 5.0f;
    EXPECT_FLOAT_EQ(state.positions[0].y(), 5.0f);
}

// massVector returns view of mass array
TEST(MechanicalStateBulkTest, MassVector) {
    MechanicalState state;
    state.resize(3);

    state.setMass(0, 1.0f);
    state.setMass(1, 2.0f);
    state.setMass(2, 3.0f);

    auto m = state.massVector();

    EXPECT_EQ(m.size(), 3);
    EXPECT_FLOAT_EQ(m(0), 1.0f);
    EXPECT_FLOAT_EQ(m(1), 2.0f);
    EXPECT_FLOAT_EQ(m(2), 3.0f);
}

// invMassVector for constraint projection
TEST(MechanicalStateBulkTest, InvMassVector) {
    MechanicalState state;
    state.resize(3);

    state.setMass(0, 1.0f);
    state.setMass(1, 2.0f);
    state.pin(2);  // Pinned vertex

    auto w = state.invMassVector();

    EXPECT_FLOAT_EQ(w(0), 1.0f);
    EXPECT_FLOAT_EQ(w(1), 0.5f);
    EXPECT_FLOAT_EQ(w(2), 0.0f);
}

// ============================================================================
// Integration Example: Simple Euler Step
// ============================================================================

// Demonstrate a basic explicit Euler integration step
TEST(MechanicalStateIntegrationTest, SimpleEulerStep) {
    MechanicalState state;
    state.resize(2);

    // Vertex 0: free particle at origin, moving right
    state.positions[0] = Vector3r(0, 0, 0);
    state.velocities[0] = Vector3r(1, 0, 0);
    state.setMass(0, 1.0f);

    // Vertex 1: pinned particle
    state.positions[1] = Vector3r(1, 0, 0);
    state.velocities[1] = Vector3r(0, 0, 0);
    state.pin(1);

    // Apply gravity
    Vector3r gravity(0, -9.8f, 0);
    Real dt = 0.01f;

    // Euler step for each particle
    for (int i = 0; i < state.size(); ++i) {
        if (state.getInvMass(i) > 0) {
            // v += a * dt (where a = gravity for free particles)
            state.velocities[i] += gravity * dt;
            // x += v * dt
            state.positions[i] += state.velocities[i] * dt;
        }
    }

    // Free particle moved
    EXPECT_GT(state.positions[0].x(), 0.0f);
    EXPECT_LT(state.positions[0].y(), 0.0f);  // Fell due to gravity

    // Pinned particle didn't move
    EXPECT_FLOAT_EQ(state.positions[1].x(), 1.0f);
    EXPECT_FLOAT_EQ(state.positions[1].y(), 0.0f);
}
