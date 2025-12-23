// components.h

#pragma once

#include <Eigen/Dense>
#include <vector>

namespace geometry {

    // flags
    struct PARTICLE_FLAG_ACTIVE {};
    struct SHAPE_FLAG_COLLIDE_PARTICLES {};

    // types
    enum GEOMETRY_TYPE {
        GEO_SPHERE,
        GEO_BOX,
        GEO_CAPSULE,
        GEO_CYLINDER,
        GEO_CONE,
        GEO_MESH,
        GEO_SDF,
        GEO_PLANE,
        GEO_NONE
    };

    struct SDF {
        int volume;
        Eigen::Matrix3f I;
        float mass;
        Eigen::Vector3f com;
        bool has_inertia;
        bool is_solid;
    };

    struct Mesh {
        std::vector<Eigen::Vector3f> vertices;
        std::vector<int> indices;
        Eigen::Matrix3f I;
        float mass;
        Eigen::Vector3f com;
        bool has_inertia;
        bool is_solid;
    };
} // namespace geometry
