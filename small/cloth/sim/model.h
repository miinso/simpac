#pragma once

#include "state.h"
#include <vector>
#include <cassert>
#include <cstdint>

namespace physics {

// matches warp's
constexpr uint32_t PARTICLE_FLAG_ACTIVE = 1u << 0;

// static topology + material properties, rebuilt on structural change
struct Model {
    int particle_count = 0;
    int spring_count = 0;
    int tri_count = 0;
    int edge_count = 0;

    // initial particle state (cloned into State)
    Eigen::VectorXr particle_q;          // [n * 3]
    Eigen::VectorXr particle_qd;         // [n * 3]

    // per-particle static
    Eigen::VectorXr particle_mass;       // [n]
    Eigen::VectorXr particle_inv_mass;   // [n]
    std::vector<uint32_t> particle_flags; // [n], PARTICLE_FLAG_ACTIVE etc.
    std::vector<int> free_particles;     // indices of non-pinned particles (inv_mass > 0)

    // springs
    std::vector<int> spring_indices;     // [spring_count * 2]
    Eigen::VectorXr spring_rest_length;  // [spring_count]
    Eigen::VectorXr spring_stiffness;    // [spring_count]
    Eigen::VectorXr spring_damping;      // [spring_count]

    // triangles (FEM)
    std::vector<int> tri_indices;        // [tri_count * 3]
    std::vector<Eigen::Matrix2r> tri_poses; // dm_inv [tri_count]
    Eigen::VectorXr tri_areas;           // [tri_count]
    Eigen::VectorXr tri_mu;              // [tri_count]
    Eigen::VectorXr tri_lambda;          // [tri_count]
    Eigen::VectorXr tri_thickness;       // [tri_count]

    // edges (bending)
    std::vector<int> edge_indices;       // [edge_count * 4]: o0, o1, v1, v2
    Eigen::VectorXr edge_rest_angle;     // [edge_count]
    Eigen::VectorXr edge_bending_stiffness; // [edge_count]
    Eigen::VectorXr edge_bending_damping;   // [edge_count]

    // create a fresh state from this model
    State state() const {
        State s;
        s.particle_count = particle_count;
        s.particle_q = particle_q;
        s.particle_qd = particle_qd;
        s.particle_f.setZero(particle_count * 3);
        return s;
    }
};

// accumulates particles/springs/tris/edges, then finalize() -> Model
struct ModelBuilder {
    std::vector<Eigen::Vector3r> particle_q;
    std::vector<Eigen::Vector3r> particle_qd;
    std::vector<Real> particle_mass;
    std::vector<uint32_t> particle_flags;

    std::vector<int> spring_indices;
    std::vector<Real> spring_rest_length;
    std::vector<Real> spring_stiffness;
    std::vector<Real> spring_damping;

    std::vector<int> tri_indices;
    std::vector<Eigen::Matrix2r> tri_poses;
    std::vector<Real> tri_areas;
    std::vector<Real> tri_mu;
    std::vector<Real> tri_lambda;
    std::vector<Real> tri_thickness;

    std::vector<int> edge_indices;
    std::vector<Real> edge_rest_angle;
    std::vector<Real> edge_bending_stiffness;
    std::vector<Real> edge_bending_damping;

    // returns particle index
    int add_particle(const Eigen::Vector3r& pos,
                     const Eigen::Vector3r& vel,
                     Real mass,
                     uint32_t flags = PARTICLE_FLAG_ACTIVE) {
        int idx = (int)particle_q.size();
        particle_q.push_back(pos);
        particle_qd.push_back(vel);
        particle_mass.push_back(mass);
        particle_flags.push_back(flags);
        return idx;
    }

    // rest_length computed from positions unless specified
    void add_spring(int i, int j, Real stiffness, Real damping,
                    Real rest_length = -1) {
        assert(i >= 0 && i < (int)particle_q.size());
        assert(j >= 0 && j < (int)particle_q.size());
        spring_indices.push_back(i);
        spring_indices.push_back(j);
        spring_rest_length.push_back(
            rest_length >= 0 ? rest_length : (particle_q[i] - particle_q[j]).norm());
        spring_stiffness.push_back(stiffness);
        spring_damping.push_back(damping);
    }

    // computes dm_inv + area from rest positions
    void add_triangle(int i, int j, int k,
                      Real mu, Real lam,
                      Real thickness = 1) {
        assert(i >= 0 && i < (int)particle_q.size());
        assert(j >= 0 && j < (int)particle_q.size());
        assert(k >= 0 && k < (int)particle_q.size());

        const auto& x0 = particle_q[i];
        const auto& x1 = particle_q[j];
        const auto& x2 = particle_q[k];

        Eigen::Vector3r e1 = x1 - x0;
        Eigen::Vector3r e2 = x2 - x0;

        Real area = 0.5f * e1.cross(e2).norm();

        // local 2D frame for material coordinates
        Eigen::Vector3r t1 = e1.normalized();
        Eigen::Vector3r t2 = e1.cross(e2).normalized().cross(t1);

        Eigen::Matrix2r Dm;
        Dm(0, 0) = e1.dot(t1);
        Dm(1, 0) = e1.dot(t2);
        Dm(0, 1) = e2.dot(t1);
        Dm(1, 1) = e2.dot(t2);

        add_triangle(i, j, k, Dm.inverse(), area, mu, lam, thickness);
    }

    // pre-computed dm_inv + area (for existing triangles with known rest pose)
    void add_triangle(int i, int j, int k,
                      const Eigen::Matrix2r& dm_inv, Real area,
                      Real mu, Real lam,
                      Real thickness = 1) {
        tri_indices.push_back(i);
        tri_indices.push_back(j);
        tri_indices.push_back(k);
        tri_poses.push_back(dm_inv);
        tri_areas.push_back(area);
        tri_mu.push_back(mu);
        tri_lambda.push_back(lam);
        tri_thickness.push_back(thickness);
    }

    void add_edge(int o0, int o1, int v1, int v2,
                  Real rest_angle, Real stiffness,
                  Real damping = 0) {
        edge_indices.push_back(o0);
        edge_indices.push_back(o1);
        edge_indices.push_back(v1);
        edge_indices.push_back(v2);
        edge_rest_angle.push_back(rest_angle);
        edge_bending_stiffness.push_back(stiffness);
        edge_bending_damping.push_back(damping);
    }

    Model finalize() const {
        Model m;
        const int n = (int)particle_q.size();
        const int ns = (int)spring_rest_length.size();
        const int nt = (int)tri_areas.size();
        const int ne = (int)edge_rest_angle.size();

        m.particle_count = n;
        m.spring_count = ns;
        m.tri_count = nt;
        m.edge_count = ne;

        // flatten particle vec3s into contiguous VectorXr
        m.particle_q.resize(n * 3);
        m.particle_qd.resize(n * 3);
        for (int i = 0; i < n; i++) {
            m.particle_q.segment<3>(i * 3) = particle_q[i];
            m.particle_qd.segment<3>(i * 3) = particle_qd[i];
        }

        m.particle_mass = Eigen::Map<const Eigen::VectorXr>(particle_mass.data(), n);
        m.particle_inv_mass.resize(n);
        for (int i = 0; i < n; i++) {
            m.particle_inv_mass[i] = particle_mass[i] > 0
                ? 1 / particle_mass[i] : 0;
            if (m.particle_inv_mass[i] > 0)
                m.free_particles.push_back(i);
        }
        m.particle_flags = particle_flags;

        // springs
        m.spring_indices = spring_indices;
        if (ns > 0) {
            m.spring_rest_length = Eigen::Map<const Eigen::VectorXr>(spring_rest_length.data(), ns);
            m.spring_stiffness = Eigen::Map<const Eigen::VectorXr>(spring_stiffness.data(), ns);
            m.spring_damping = Eigen::Map<const Eigen::VectorXr>(spring_damping.data(), ns);
        }

        // triangles
        m.tri_indices = tri_indices;
        m.tri_poses = tri_poses;
        if (nt > 0) {
            m.tri_areas = Eigen::Map<const Eigen::VectorXr>(tri_areas.data(), nt);
            m.tri_mu = Eigen::Map<const Eigen::VectorXr>(tri_mu.data(), nt);
            m.tri_lambda = Eigen::Map<const Eigen::VectorXr>(tri_lambda.data(), nt);
            m.tri_thickness = Eigen::Map<const Eigen::VectorXr>(tri_thickness.data(), nt);
        }

        // edges
        m.edge_indices = edge_indices;
        if (ne > 0) {
            m.edge_rest_angle = Eigen::Map<const Eigen::VectorXr>(edge_rest_angle.data(), ne);
            m.edge_bending_stiffness = Eigen::Map<const Eigen::VectorXr>(edge_bending_stiffness.data(), ne);
            m.edge_bending_damping = Eigen::Map<const Eigen::VectorXr>(edge_bending_damping.data(), ne);
        }

        return m;
    }
};

} // namespace physics
