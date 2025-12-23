#pragma once

#include "simplicial.h"

namespace simplicial {

// ============================================================================
// MechanicalState - Per-Vertex DOF State
// ============================================================================
// Separated from topology following SOFA pattern.
// Topology remains fixed; positions can deform.

struct MechanicalState {
    // Position state
    std::vector<Vector3r> positions;       // x - current positions
    std::vector<Vector3r> rest_positions;  // x0 - rest/reference positions
    std::vector<Vector3r> old_positions;   // x_old - for velocity computation

    // Velocity/dynamics state
    std::vector<Vector3r> velocities;      // v
    std::vector<Vector3r> accelerations;   // a (or forces before mass division)

    // Mass state
    std::vector<Real> masses;
    std::vector<Real> inv_masses;  // Precomputed 1/m, 0 for pinned vertices

    // ========================================================================
    // Query
    // ========================================================================

    int size() const { return static_cast<int>(positions.size()); }

    // ========================================================================
    // Initialization
    // ========================================================================

    void resize(int n) {
        positions.resize(n, Vector3r::Zero());
        rest_positions.resize(n, Vector3r::Zero());
        old_positions.resize(n, Vector3r::Zero());
        velocities.resize(n, Vector3r::Zero());
        accelerations.resize(n, Vector3r::Zero());
        masses.resize(n, Real(1));
        inv_masses.resize(n, Real(1));
    }

    void reserve(int n) {
        positions.reserve(n);
        rest_positions.reserve(n);
        old_positions.reserve(n);
        velocities.reserve(n);
        accelerations.reserve(n);
        masses.reserve(n);
        inv_masses.reserve(n);
    }

    void clear() {
        positions.clear();
        rest_positions.clear();
        old_positions.clear();
        velocities.clear();
        accelerations.clear();
        masses.clear();
        inv_masses.clear();
    }

    // ========================================================================
    // Mass Operations
    // ========================================================================

    void setMass(int i, Real mass) {
        masses[i] = mass;
        inv_masses[i] = (mass != Real(0)) ? Real(1) / mass : Real(0);
    }

    // Pin vertex (infinite mass, won't move)
    void pin(int i) {
        inv_masses[i] = Real(0);
    }

    // Unpin vertex with given mass
    void unpin(int i, Real mass) {
        setMass(i, mass);
    }

    // ========================================================================
    // State Operations
    // ========================================================================

    // Copy current positions to rest positions
    void setRestFromCurrent() {
        rest_positions = positions;
    }

    // Save current to old (for velocity computation)
    void saveToOld() {
        old_positions = positions;
    }

    // Compute velocities from position difference
    void computeVelocities(Real dt) {
        Real inv_dt = Real(1) / dt;
        for (int i = 0; i < size(); ++i) {
            velocities[i] = (positions[i] - old_positions[i]) * inv_dt;
        }
    }

    // Reset accelerations to zero
    void clearAccelerations() {
        for (auto& a : accelerations) {
            a.setZero();
        }
    }

    // ========================================================================
    // Accessors (matching PBD::ParticleData interface)
    // ========================================================================

    Vector3r& getPosition(int i) { return positions[i]; }
    const Vector3r& getPosition(int i) const { return positions[i]; }

    void setPosition(int i, const Vector3r& pos) { positions[i] = pos; }

    Vector3r& getPosition0(int i) { return rest_positions[i]; }
    const Vector3r& getPosition0(int i) const { return rest_positions[i]; }

    void setPosition0(int i, const Vector3r& pos) { rest_positions[i] = pos; }

    Vector3r& getOldPosition(int i) { return old_positions[i]; }
    const Vector3r& getOldPosition(int i) const { return old_positions[i]; }

    Vector3r& getVelocity(int i) { return velocities[i]; }
    const Vector3r& getVelocity(int i) const { return velocities[i]; }

    void setVelocity(int i, const Vector3r& vel) { velocities[i] = vel; }

    Vector3r& getAcceleration(int i) { return accelerations[i]; }
    const Vector3r& getAcceleration(int i) const { return accelerations[i]; }

    void setAcceleration(int i, const Vector3r& acc) { accelerations[i] = acc; }

    Real getMass(int i) const { return masses[i]; }
    Real getInvMass(int i) const { return inv_masses[i]; }

    // ========================================================================
    // Bulk Operations (for FEM solvers)
    // ========================================================================

    // Get all positions as Eigen map for vectorized operations
    // Usage: auto x = state.positionMatrix(); // Nx3 matrix view
    Eigen::Map<Eigen::Matrix<Real, Eigen::Dynamic, 3, Eigen::RowMajor>>
    positionMatrix() {
        return Eigen::Map<Eigen::Matrix<Real, Eigen::Dynamic, 3, Eigen::RowMajor>>(
            positions[0].data(), size(), 3);
    }

    Eigen::Map<const Eigen::Matrix<Real, Eigen::Dynamic, 3, Eigen::RowMajor>>
    positionMatrix() const {
        return Eigen::Map<const Eigen::Matrix<Real, Eigen::Dynamic, 3, Eigen::RowMajor>>(
            positions[0].data(), size(), 3);
    }

    Eigen::Map<Eigen::Matrix<Real, Eigen::Dynamic, 3, Eigen::RowMajor>>
    restPositionMatrix() {
        return Eigen::Map<Eigen::Matrix<Real, Eigen::Dynamic, 3, Eigen::RowMajor>>(
            rest_positions[0].data(), size(), 3);
    }

    Eigen::Map<const Eigen::Matrix<Real, Eigen::Dynamic, 3, Eigen::RowMajor>>
    restPositionMatrix() const {
        return Eigen::Map<const Eigen::Matrix<Real, Eigen::Dynamic, 3, Eigen::RowMajor>>(
            rest_positions[0].data(), size(), 3);
    }

    Eigen::Map<Eigen::Matrix<Real, Eigen::Dynamic, 3, Eigen::RowMajor>>
    velocityMatrix() {
        return Eigen::Map<Eigen::Matrix<Real, Eigen::Dynamic, 3, Eigen::RowMajor>>(
            velocities[0].data(), size(), 3);
    }

    Eigen::Map<Eigen::VectorX<Real>> massVector() {
        return Eigen::Map<Eigen::VectorX<Real>>(masses.data(), size());
    }

    Eigen::Map<Eigen::VectorX<Real>> invMassVector() {
        return Eigen::Map<Eigen::VectorX<Real>>(inv_masses.data(), size());
    }
};

} // namespace simplicial
